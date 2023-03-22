#include "lib_http_client_websocket.h"
#include "../util.h"
#include <log.h>
#include <curl/curl.h>
#include <thread>
#include <unistd.h>

HttpClientWebSocket::HttpClientWebSocket(FakeJni::JLong owner) {
    Log::debug("HTTPClientWebSocket", "owner: %ld", owner);
    HttpClientWebSocket::owner = owner;
    curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecb);
    //curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);

    jvm = (void*)&FakeJni::JniEnvContext().getJniEnv().getVM();
    Log::trace("HTTPClientWebSocket", "init called");
}
HttpClientWebSocket::~HttpClientWebSocket() {
    //curl_easy_cleanup(curl);
    Log::trace("HTTPClientWebSocket", "deinit called");
}
void HttpClientWebSocket::connect(std::shared_ptr<FakeJni::JString> url, std::shared_ptr<FakeJni::JString> wst) {
    Log::trace("HTTPClientWebSocket", "connect called: %s, %s", url->asStdString().c_str(), wst->asStdString().c_str());
    // auto self = shared_from_this();
    std::thread([=]() {
    curl_easy_setopt(curl, CURLOPT_URL, url->asStdString().c_str());
    header = curl_slist_append(header, ("Sec-WebSocket-Protocol: " + wst->asStdString()).data());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    auto ret = curl_easy_perform(curl);
    if(ret == CURLE_OK) {
        Log::debug("HTTPClientWebSocket", "Opening conenction success");
    } else {
        Log::error("HTTPClientWebSocket", "Opening conenction failed");
    }
    }).detach();
    //char buffer[256];
    //size_t rlen;

  //struct curl_ws_frame *meta;

      //CURLcode result = curl_ws_recv(curl, buffer, sizeof(buffer), &rlen, &meta);
      //Log::info("", "%d", meta->flags);
      //printf(buffer);
}
void HttpClientWebSocket::addHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value) {
    header = curl_slist_append(header, (name->asStdString() + ": " + value->asStdString()).c_str());

    Log::info("HTTPClientWebSocket", "WebSocket stub addHeader called: %s, %s", name->asStdString().c_str(), value->asStdString().c_str());
}
FakeJni::JBoolean HttpClientWebSocket::sendMessage(std::shared_ptr<FakeJni::JString> foo) {
        Log::trace("HTTPClientWebSocket", "sendMessage called: %s", foo->asStdString().c_str());
    size_t sent = 0;
    curl_ws_send(curl, foo->asStdString().c_str(), foo->asStdString().length(), &sent, 0, CURLWS_TEXT);
    return true;
}
FakeJni::JBoolean HttpClientWebSocket::sendBinaryMessage(std::shared_ptr<jnivm::ByteBuffer> foo) {
        Log::trace("HTTPClientWebSocket", "WebSocket stub sendBinaryMessage called");
    return true;
}
void HttpClientWebSocket::disconnect(int id) {
    Log::trace("HTTPClientWebSocket", "WebSocket stub disconnect called");
}

size_t HttpClientWebSocket::write_callback(char *ptr, size_t size, size_t nmemb) {
    if(!connected) {
        connected = true;
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("()V", "onOpen");
        method->invoke(frame.getJniEnv(), this);
    }
    auto frame = curl_ws_meta(curl);
    Log::trace("HTTPClientWebSocket", "curl_ws_meta: age %d bytesleft %lld offset %lld flags %d", frame->age, (long long)frame->bytesleft, (long long)frame->offset, frame->flags);
    if(frame->flags & CURLWS_TEXT) {
        std::string cont((const char*)ptr, nmemb);
        Log::trace("HTTPClientWebSocket", "length %lld len %lld content: %s", (long long) nmemb, (long long)frame->len, cont.c_str());
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("(Ljava/lang/String;)V", "onMessage");
        method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(cont)));
    }
    return nmemb;
}


size_t HttpClientWebSocket::writecb(char *buffer, size_t size, size_t nitems, void *userdata)
{
//     Log::trace("HTTPClientWebSocket", "Got data");
//   size_t i;
//   size_t incoming = nitems;
//   struct curl_ws_frame *meta;
//   (void)size;
//   for(i = 0; i < nitems; i++)
//     printf("%02x ", (unsigned char)buffer[i]);
//   printf("\n");
//   //HttpClientWebSocket wscli;
//   //wscli.write_cb2(nitems);
  

  return ((HttpClientWebSocket*)userdata)->write_callback(buffer, size, nitems);
}

size_t HttpClientWebSocket::write_cb2(size_t nitems) {
    //    auto&& jvm = &frame.getJniEnv().getVM();
    //FakeJni::LocalFrame frame1(*jvm);

    //auto method = getClass().getMethod("(Ljava/lang/String;)V", "onMessage");
    //method->invoke(frame.getJniEnv(), this, call_handle, frame.getJniEnv().createLocalReference(this->shared_from_this()), frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>("Got message")));
    return nitems;

}