#include "lib_http_client_websocket.h"
#include "../util.h"
#include <log.h>
#include <curl/curl.h>
#include <thread>
#include <unistd.h>

HttpClientWebSocket::HttpClientWebSocket(FakeJni::JLong owner) {
    HttpClientWebSocket::owner = owner;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_PREREQDATA, this);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, this);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecb);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, closesocket);
    curl_easy_setopt(curl, CURLOPT_PREREQFUNCTION, prereq_callback);

    jvm = (void*)&FakeJni::JniEnvContext().getJniEnv().getVM();
}
HttpClientWebSocket::~HttpClientWebSocket() {
    //curl_easy_cleanup(curl);
}

void HttpClientWebSocket::connect(std::shared_ptr<FakeJni::JString> url, std::shared_ptr<FakeJni::JString> wst) {
    std::thread([=]() {
    curl_easy_setopt(curl, CURLOPT_URL, url->asStdString().c_str());
    header = curl_slist_append(header, ("Sec-WebSocket-Protocol: " + wst->asStdString()).data());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    auto ret = curl_easy_perform(curl);
    if(ret == CURLE_OK) {
    } else {
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("()V", "onFailure");
        method->invoke(frame.getJniEnv(), this);
        Log::error("HTTPClientWebSocket", "Opening websocket conenction failed");
    }
    }).detach();
}

void HttpClientWebSocket::addHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value) {
    header = curl_slist_append(header, (name->asStdString() + ": " + value->asStdString()).c_str());
}

FakeJni::JBoolean HttpClientWebSocket::sendMessage(std::shared_ptr<FakeJni::JString> foo) {
    size_t sent = 0;
    curl_ws_send(curl, foo->asStdString().c_str(), foo->asStdString().length(), &sent, 0, CURLWS_TEXT);
    return true;
}

FakeJni::JBoolean HttpClientWebSocket::sendBinaryMessage(std::shared_ptr<jnivm::ByteBuffer> foo) {
    return true;
}

void HttpClientWebSocket::disconnect(int id) {
}

size_t HttpClientWebSocket::write_callback(char *ptr, size_t size, size_t nmemb) {
    auto frame = curl_ws_meta(curl);
    if(frame->flags & CURLWS_TEXT) {
        std::string cont((const char*)ptr, nmemb);
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("(Ljava/lang/String;)V", "onMessage");
        method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(cont)));
    }
    return nmemb;
}


size_t HttpClientWebSocket::writecb(char *buffer, size_t size, size_t nitems, void *userdata)
{
  return ((HttpClientWebSocket*)userdata)->write_callback(buffer, size, nitems);
}

int HttpClientWebSocket::prereq_callback(void *clientp, char *conn_primary_ip, char *conn_local_ip, int conn_primary_port, int conn_local_port) {
    ((HttpClientWebSocket*)clientp)->sendOpened();
return CURL_PREREQFUNC_OK;
}

void HttpClientWebSocket::sendOpened() {
    if(!connected) {
        connected = true;
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("()V", "onOpen");
        method->invoke(frame.getJniEnv(), this);
    }
}

int HttpClientWebSocket::closesocket(void *clientp, curl_socket_t item) {
    ((HttpClientWebSocket*)clientp)->sendClosed();
    return 0;
}

void HttpClientWebSocket::sendClosed() {
    FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
    auto method = getClass().getMethod("(I)V", "onClose");
    method->invoke(frame.getJniEnv(), this, 0);
    connected = false;
}
