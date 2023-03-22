#include <fake-jni/fake-jni.h>
#include "main_activity.h"
#include <log.h>
#include "lib_http_client.h"
#include <utility>
#include <jnivm/bytebuffer.h>

class HttpClientWebSocket : public FakeJni::JObject {
    FakeJni::JLong owner = 0;
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientWebSocket")
    HttpClientWebSocket(FakeJni::JLong owner);
    ~HttpClientWebSocket();
    void connect(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
    void addHeader(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
    FakeJni::JBoolean sendMessage(std::shared_ptr<FakeJni::JString>);
    FakeJni::JBoolean sendBinaryMessage(std::shared_ptr<jnivm::ByteBuffer>);
    void disconnect(int id);
    static size_t write_callback_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *self = static_cast<HttpClientWebSocket *>(userdata);
        return self->write_callback(ptr, size, nmemb);
    }
    
private:
    void *curl;
    void *jvm;
    bool connected = false;
    FakeJni::JLong call_handle;
    struct curl_slist *header = nullptr;
    size_t write_callback(char *ptr, size_t size, size_t nmemb);
    static size_t writecb(char *buffer, size_t size, size_t nitems, void* data);
    size_t write_cb2(size_t nitems);
    std::vector<signed char> response;
    std::vector<ResponseHeader> headers;
};