#include <fake-jni/fake-jni.h>
#include "main_activity.h"
#include <log.h>

#include <utility>

class HttpClientWebSocket : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientWebSocket")
    void connect(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
private:
    FakeJni::JLong call_handle;
};
