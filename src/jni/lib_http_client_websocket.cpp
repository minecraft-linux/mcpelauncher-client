#include "lib_http_client_websocket.h"
#include "../util.h"
#include <log.h>
#include <curl/curl.h>
#include <thread>
#include <unistd.h>

#if defined(CURLWS_TEXT) && defined(CURLWS_BINARY)
#define ENABLE_WEBSOCKETS
#endif

HttpClientWebSocket::HttpClientWebSocket(FakeJni::JLong owner) {
    HttpClientWebSocket::owner = owner;

#ifdef ENABLE_WEBSOCKETS
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecb);
#endif

    jvm = (void*)&FakeJni::JniEnvContext().getJniEnv().getVM();
}
HttpClientWebSocket::~HttpClientWebSocket() {
#ifdef ENABLE_WEBSOCKETS
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);
#endif
}

void HttpClientWebSocket::connect(std::shared_ptr<FakeJni::JString> url, std::shared_ptr<FakeJni::JString> wst) {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "connect called, url: %s, wst: %s", url->asStdString().c_str(), wst->asStdString().c_str());
#endif
    std::thread([=]() {
#ifdef ENABLE_WEBSOCKETS
        curl_easy_setopt(curl, CURLOPT_URL, url->asStdString().c_str());
        header = curl_slist_append(header, ("Sec-WebSocket-Protocol: " + wst->asStdString()).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        auto ret = curl_easy_perform(curl);
        if(ret == CURLE_OK) {
#ifndef NDEBUG
            Log::trace("HTTPClientWebSocket", "websocket connection closed");
#endif
            sendClosed();
        } else {
            Log::error("HTTPClientWebSocket", "websocket connection closed with an error");
            connected = false;
            FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
            auto method = getClass().getMethod("()V", "onFailure");
            method->invoke(frame.getJniEnv(), this);
        }
#else
        Log::error("HTTPClientWebSocket", "Missing curl websocket support");
        sendOpened();
#endif
    }).detach();
}

void HttpClientWebSocket::addHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value) {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "addHeader called, name: %s, value: %s", name->asStdString().c_str(), value->asStdString().c_str());
#endif
    header = curl_slist_append(header, (name->asStdString() + ": " + value->asStdString()).c_str());
}

FakeJni::JBoolean HttpClientWebSocket::sendMessage(std::shared_ptr<FakeJni::JString> msg) {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "sendMessage called, message: %s", msg->asStdString().c_str());
#endif
#ifdef ENABLE_WEBSOCKETS
    if(!connected) {
        return false;
    }
    size_t sent = 0;
    curl_ws_send(curl, msg->asStdString().c_str(), msg->asStdString().length(), &sent, 0, CURLWS_TEXT);
#endif
    return true;
}

FakeJni::JBoolean HttpClientWebSocket::sendBinaryMessage(std::shared_ptr<jnivm::ByteBuffer> msg) {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "sendBinaryMessage called");
#endif
#ifdef ENABLE_WEBSOCKETS
    if(!connected) {
        return false;
    }
    size_t sent = 0;
    curl_ws_send(curl, msg->buffer, msg->capacity, &sent, 0, CURLWS_BINARY);
#endif
    return true;
}

void HttpClientWebSocket::disconnect(int id) {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "disconnect called, id: %d", id);
#endif
#ifdef ENABLE_WEBSOCKETS
    connected = false;
    size_t sent;
    curl_ws_send(curl, "", 0, &sent, 0, CURLWS_CLOSE);
#endif
}

size_t HttpClientWebSocket::write_callback(char *ptr, size_t size, size_t nmemb) {
#ifdef ENABLE_WEBSOCKETS
    if(!connected) {
        sendOpened();
    }
    auto frame = curl_ws_meta(curl);
    if(frame->flags & CURLWS_TEXT) {
        std::string cont((const char*)ptr, nmemb);
#ifndef NDEBUG
        Log::trace("HttpClientWebSocket", "Got message: %s", cont.c_str());
#endif
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("(Ljava/lang/String;)V", "onMessage");
        method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(cont)));
    } else if(frame->flags & CURLWS_BINARY) {
#ifndef NDEBUG
        Log::trace("HttpClientWebSocket", "Got binary message");
#endif
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("(Ljava/nio/ByteBuffer;)V", "onBinaryMessage");
        method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<jnivm::ByteBuffer>(ptr, nmemb)));
    }
#endif
    return nmemb;
}


size_t HttpClientWebSocket::writecb(char *buffer, size_t size, size_t nitems, void *userdata) {
    return ((HttpClientWebSocket*)userdata)->write_callback(buffer, size, nitems);
}

void HttpClientWebSocket::sendOpened() {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "Sending onOpen");
#endif
    FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
    auto method = getClass().getMethod("()V", "onOpen");
    method->invoke(frame.getJniEnv(), this);
    connected = true;
}

void HttpClientWebSocket::sendClosed() {
#ifndef NDEBUG
    Log::trace("HttpClientWebSocket", "Sending onClose");
#endif
    connected = false;
    FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
    auto method = getClass().getMethod("(I)V", "onClose");
    method->invoke(frame.getJniEnv(), this, 0);
}
