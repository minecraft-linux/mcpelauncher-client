#include "lib_http_client.h"

Ecdsa::Ecdsa() = default;

HttpClientRequest::HttpClientRequest() = default;

FakeJni::JBoolean HttpClientRequest::isNetworkAvailable(std::shared_ptr<Context> context) {
    return true;
}

std::shared_ptr<HttpClientRequest> HttpClientRequest::createClientRequest() {
    return std::make_shared<HttpClientRequest>(HttpClientRequest());
}

void HttpClientRequest::setHttpUrl(std::shared_ptr<FakeJni::JString> url) {

}

void HttpClientRequest::setHttpMethodAndBody(std::shared_ptr<FakeJni::JString> method,
                                             std::shared_ptr<FakeJni::JString> contentType,
                                             std::shared_ptr<FakeJni::JByteArray> body) {

}

void HttpClientRequest::setHttpHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value) {

}

void HttpClientRequest::doRequestAsync(FakeJni::JLong sourceCall) {

}

FakeJni::JInt HttpClientResponse::getNumHeaders() {
    return 0;
}

std::shared_ptr<FakeJni::JString> HttpClientResponse::getHeaderNameAtIndex(FakeJni::JInt index) {
    return std::shared_ptr<FakeJni::JString>();
}

std::shared_ptr<FakeJni::JString> HttpClientResponse::getHeaderValueAtIndex(FakeJni::JInt index) {
    return std::shared_ptr<FakeJni::JString>();
}

std::shared_ptr<FakeJni::JByteArray> HttpClientResponse::getResponseBodyBytes() {
    return std::shared_ptr<FakeJni::JByteArray>();
}

FakeJni::JInt HttpClientResponse::getResponseCode() {
    return 200;
}
