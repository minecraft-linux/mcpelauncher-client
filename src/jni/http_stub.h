#pragma once

#include <fake-jni/fake-jni.h>
#include "java_types.h"

class Header : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("org/apache/http/Header")
};

class HTTPResponse : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/android/net/HTTPResponse")
    jint getStatus();
    std::shared_ptr<FakeJni::JString> getBody();
    jint getResponseCode();
    std::shared_ptr<FakeJni::JArray<Header>> getHeaders();
};
class HTTPRequest : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/android/net/HTTPRequest")
    HTTPRequest();
    void setURL(std::shared_ptr<FakeJni::JString>);
    void setRequestBody(std::shared_ptr<FakeJni::JString>);
    void setCookieData(std::shared_ptr<FakeJni::JString>);
    void setContentType(std::shared_ptr<FakeJni::JString>);
    std::shared_ptr<HTTPResponse> send(std::shared_ptr<FakeJni::JString>);
    void abort();
};
