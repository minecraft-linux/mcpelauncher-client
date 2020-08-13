#pragma once

#include <fake-jni/fake-jni.h>
#include "main_activity.h"

class Ecdsa : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/Ecdsa")
    Ecdsa();
};

class HttpClientRequest : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientRequest")
    HttpClientRequest();

    static FakeJni::JBoolean isNetworkAvailable(std::shared_ptr<Context> context);
    static std::shared_ptr<HttpClientRequest> createClientRequest();

    void setHttpUrl(std::shared_ptr<FakeJni::JString> url);
    void setHttpMethodAndBody(std::shared_ptr<FakeJni::JString> method, std::shared_ptr<FakeJni::JString> contentType, std::shared_ptr<FakeJni::JByteArray> body);
    void setHttpHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value);
    void doRequestAsync(FakeJni::JLong sourceCall);
};

class HttpClientResponse : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientResponse")

    FakeJni::JInt getNumHeaders();
    std::shared_ptr<FakeJni::JString> getHeaderNameAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JString> getHeaderValueAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JByteArray> getResponseBodyBytes();
    FakeJni::JInt getResponseCode();


};
