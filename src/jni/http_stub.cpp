#include "http_stub.h"
// Never called since Minecraft 1.13
// Pre 1.13 compatibility

jint HTTPResponse::getStatus() {
    return 1;
}

std::shared_ptr<FakeJni::JString> HTTPResponse::getBody() {
    return std::make_shared<FakeJni::JString>();
}

jint HTTPResponse::getResponseCode() {
    return 200;
}

std::shared_ptr<FakeJni::JArray<Header>> HTTPResponse::getHeaders() {
    return std::make_shared<FakeJni::JArray<Header>>();
}

HTTPRequest::HTTPRequest() {
}

void HTTPRequest::setURL(std::shared_ptr<FakeJni::JString> arg0) {
}

void HTTPRequest::setRequestBody(std::shared_ptr<FakeJni::JString> arg0) {
}

void HTTPRequest::setCookieData(std::shared_ptr<FakeJni::JString> arg0) {
}

void HTTPRequest::setContentType(std::shared_ptr<FakeJni::JString> arg0) {
}

std::shared_ptr<HTTPResponse> HTTPRequest::send(std::shared_ptr<FakeJni::JString> arg0) {
    return std::make_shared<HTTPResponse>();
}

void HTTPRequest::abort() {
}
