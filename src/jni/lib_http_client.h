#pragma once

#include <fake-jni/fake-jni.h>
#include "main_activity.h"
#include <log.h>

#include <utility>

class ResponseHeader {
public:
    ResponseHeader(std::string name, std::string value) : name(std::move(name)), value(std::move(value)) {}
    std::string name;
    std::string value;
};

class NativeInputStream : public FakeJni::JObject {
    FakeJni::JLong call_handle;
    FakeJni::JLong offset = 0;
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientRequestBody/00024NativeInputStream")
    NativeInputStream(FakeJni::JLong call_handle);
    size_t Read(void* buffer, size_t size);
};

class HttpClientRequest : public FakeJni::JObject {
    std::shared_ptr<NativeInputStream> inputStream;
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientRequest")
    HttpClientRequest();
    ~HttpClientRequest();

    static FakeJni::JBoolean isNetworkAvailable(std::shared_ptr<Context> context);
    static std::shared_ptr<HttpClientRequest> createClientRequest();

    void setHttpUrl(std::shared_ptr<FakeJni::JString> url);
    void setHttpMethodAndBody(std::shared_ptr<FakeJni::JString> method, std::shared_ptr<FakeJni::JString> contentType, std::shared_ptr<FakeJni::JByteArray> body);
    void setHttpMethodAndBody2(std::shared_ptr<FakeJni::JString>, FakeJni::JLong, std::shared_ptr<FakeJni::JString>, FakeJni::JLong);
    void setHttpHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value);
    void doRequestAsync(FakeJni::JLong sourceCall);
    static size_t write_callback_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *self = static_cast<HttpClientRequest*>(userdata);
        return self->write_callback(ptr, size, nmemb);
    }
    static size_t header_callback_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *self = static_cast<HttpClientRequest*>(userdata);
        return self->header_callback(ptr, size, nmemb);
    }

private:
    void *curl;
    struct curl_slist * header = nullptr;
    std::vector<signed char> response;
    std::vector<ResponseHeader> headers;
    std::vector<char> body;
    std::string method;

    size_t write_callback(char *ptr, size_t size, size_t nmemb);
    size_t header_callback(char *buffer,   size_t size,   size_t nitems);
};

class HttpClientResponse : public FakeJni::JObject {
    FakeJni::JLong call_handle;
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientResponse")

    HttpClientResponse(FakeJni::JLong call_handle, int response_code, std::vector<signed char> body, std::vector<ResponseHeader> headers);
    FakeJni::JInt getNumHeaders();
    std::shared_ptr<FakeJni::JString> getHeaderNameAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JString> getHeaderValueAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JByteArray> getResponseBodyBytes();
    void getResponseBodyBytes2();
    FakeJni::JInt getResponseCode();

private:
    int response_code;
    std::vector<signed char> body;
    std::vector<ResponseHeader> headers;
};

class NativeOutputStream : public FakeJni::JObject {
    FakeJni::JLong call_handle;
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientResponse/00024NativeOutputStream")
    NativeOutputStream(FakeJni::JLong call_handle);
    void WriteAll(std::shared_ptr<FakeJni::JByteArray> data);
};
