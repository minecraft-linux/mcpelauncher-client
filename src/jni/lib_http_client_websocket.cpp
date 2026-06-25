#include "lib_http_client_websocket.h"
#include "../util.h"
#include <log.h>
#include <curl/curl.h>
#include <thread>
#include <unistd.h>
#include <poll.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

#if defined(CURLWS_TEXT) && defined(CURLWS_BINARY)
#define ENABLE_WEBSOCKETS
#if LIBCURL_VERSION_NUM >= 0x080200
/* curl 8.2.0 and newer → const version */
#define CURL_WS_META_CONST const
#else
/* curl 8.0.x and 8.1.x → non-const version */
#define CURL_WS_META_CONST
#endif
#endif

typedef struct {
    char* data;
    size_t len;
    size_t cap;
    int type;
} ws_msg;

void ws_append(ws_msg* m, const void* src, size_t n) {
    if(m->len + n > m->cap) {
        size_t newcap = (m->len + n) * 2;
        m->data = (char*)realloc(m->data, newcap);
        m->cap = newcap;
    }
    memcpy(m->data + m->len, src, n);
    m->len += n;
}

void poll_socket(curl_socket_t sock, int timeout_ms) {
    struct pollfd pfd = {
        .fd = sock,
        .events = POLLIN | POLLERR | POLLHUP};

    poll(&pfd, 1, timeout_ms);
}

static std::string ws_format_binary_preview(const char* data, size_t len) {
    constexpr size_t previewLimit = 64;

    std::ostringstream out;
    out << "len=" << len << " hex=";
    out << std::hex << std::setfill('0');

    size_t previewLen = std::min(len, previewLimit);
    for(size_t i = 0; i < previewLen; ++i) {
        if(i != 0) {
            out << ' ';
        }
        out << std::setw(2) << (unsigned int)(unsigned char)data[i];
    }

    if(previewLen < len) {
        out << " ...";
    }

    out << " ascii=";
    for(size_t i = 0; i < previewLen; ++i) {
        unsigned char c = (unsigned char)data[i];
        if(std::isprint(c)) {
            out << (char)c;
        } else {
            out << "\\x" << std::setw(2) << (unsigned int)c;
        }
    }

    if(previewLen < len) {
        out << "...";
    }

    return out.str();
}

static std::string ws_format_text_preview(const char* data, size_t len) {
    constexpr size_t previewLimit = 256;

    std::ostringstream out;
    size_t previewLen = std::min(len, previewLimit);
    for(size_t i = 0; i < previewLen; ++i) {
        unsigned char c = (unsigned char)data[i];
        switch(c) {
            case '\0':
                out << "\\0";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            case '\\':
                out << "\\\\";
                break;
            default:
                if(std::isprint(c)) {
                    out << (char)c;
                } else {
                    out << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)c << std::dec;
                }
                break;
        }
    }

    if(previewLen < len) {
        out << "...";
    }

    return out.str();
}

HttpClientWebSocket::HttpClientWebSocket(FakeJni::JLong owner) {
    HttpClientWebSocket::owner = owner;

#ifdef ENABLE_WEBSOCKETS
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
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
            sendOpened();

            ws_msg msg = {0};

            while(connected) {
                char buf[512];
                size_t n;
                CURL_WS_META_CONST struct curl_ws_frame* meta;

                curlMu.lock();
                CURLcode rc = curl_ws_recv(curl, buf, sizeof(buf), &n, &meta);
                curl_socket_t sock;
                curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sock);
                curlMu.unlock();

                if(rc == CURLE_AGAIN) {
                    poll_socket(sock, 100);  // sleep until readable
                    continue;
                }

                if(rc != CURLE_OK) {
                    break;
                }

                if(meta->flags & CURLWS_CLOSE) {
                    break;
                }

                if(meta->flags & CURLWS_PING) {
                    continue;
                }

                int frameType = meta->flags & (CURLWS_TEXT | CURLWS_BINARY);
                if(!frameType) {
                    // Ignore non-user control frames such as unsolicited PONGs.
                    continue;
                }

                if(meta->offset == 0 && frameType && msg.len == 0) {
                    msg.type = frameType;
                }

                ws_append(&msg, buf, n);

                if(meta->bytesleft != 0) {
                    continue;
                }

                if(meta->flags & CURLWS_CONT) {
                    continue;
                }

                // full message assembled
                if(msg.type & CURLWS_TEXT) {
                    std::string cont((const char*)msg.data, msg.len);
#ifndef NDEBUG
                    std::string preview = ws_format_text_preview(cont.data(), cont.size());
                    Log::trace("HttpClientWebSocket", "Got message: len=%zu text=%.*s escaped=%s", cont.size(), (int)cont.size(), cont.data(), preview.c_str());
#endif
                    FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
                    auto method = getClass().getMethod("(Ljava/lang/String;)V", "onMessage");
                    method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(cont)));
                } else if(msg.type & CURLWS_BINARY) {
#ifndef NDEBUG
                    std::string preview = ws_format_binary_preview(msg.data, msg.len);
                    Log::trace("HttpClientWebSocket", "Got binary message: %s", preview.c_str());
#endif
                    FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
                    auto method = getClass().getMethod("(Ljava/nio/ByteBuffer;)V", "onBinaryMessage");
                    method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<jnivm::ByteBuffer>(msg.data, msg.len)));
                }
                msg.len = 0;  // reuse buffer
                msg.type = 0;
            }

            if(msg.data) {
                free(msg.data);
                msg.data = nullptr;
                msg.len = 0;
                msg.cap = 0;
            }
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
    curlMu.lock();
    curl_ws_send(curl, msg->asStdString().c_str(), msg->asStdString().length(), &sent, 0, CURLWS_TEXT);
    curlMu.unlock();
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
    curlMu.lock();
    curl_ws_send(curl, msg->buffer, msg->capacity, &sent, 0, CURLWS_BINARY);
    curlMu.unlock();
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
    curlMu.lock();
    curl_ws_send(curl, "", 0, &sent, 0, CURLWS_CLOSE);
    curlMu.unlock();
#endif
}

void HttpClientWebSocket::sendOpened() {
    if(!connected) {
#ifndef NDEBUG
        Log::trace("HttpClientWebSocket", "Sending onOpen");
#endif
        FakeJni::LocalFrame frame(*(FakeJni::Jvm*)jvm);
        auto method = getClass().getMethod("()V", "onOpen");
        method->invoke(frame.getJniEnv(), this);
        connected = true;
    }
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
