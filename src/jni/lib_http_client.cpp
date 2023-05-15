#include "lib_http_client.h"
#include "../util.h"
#include <log.h>
#include <curl/curl.h>
#include <thread>

using namespace std::placeholders;

HttpClientRequest::HttpClientRequest() {
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    // Fallback if neither setHttpMethodAndBody or setHttpMethodAndBody2 has been called
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClientRequest::write_callback_wrapper_old);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HttpClientRequest::header_callback_wrapper);
}

HttpClientRequest::~HttpClientRequest() {
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);
}

FakeJni::JBoolean HttpClientRequest::isNetworkAvailable(std::shared_ptr<Context> context) {
    return true;
}

std::shared_ptr<HttpClientRequest> HttpClientRequest::createClientRequest() {
    return std::make_shared<HttpClientRequest>();
}

void HttpClientRequest::setHttpUrl(std::shared_ptr<FakeJni::JString> url) {
#ifndef NDEBUG
    Log::trace("HttpClient", "URL: %s", url->asStdString().c_str());
#endif
    curl_easy_setopt(curl, CURLOPT_URL, url->asStdString().c_str());
}

void HttpClientRequest::setHttpMethodAndBody(std::shared_ptr<FakeJni::JString> method,
                                             std::shared_ptr<FakeJni::JString> contentType,
                                             std::shared_ptr<FakeJni::JByteArray> body) {
    this->method = method->asStdString();
    if (this->method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (this->method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (this->method == "HEAD") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, this->method.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClientRequest::write_callback_wrapper_old);
    this->body = body ? std::vector<char>((char *)body->getArray(), (char *)body->getArray() + body->getSize()) : std::vector<char>{};
    if(this->body.size()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, this->body.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, this->body.size());
    }
    auto conttype = contentType->asStdString();
    if(conttype.length()) {
        header = curl_slist_append(header, ("Content-Type: " + conttype).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    }
}

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto stream = (NativeInputStream *)userdata;
    try {
        return stream->Read(ptr, size * nmemb);
    } catch(...) {
#ifdef CURL_READFUNC_ABORT
        return CURL_READFUNC_ABORT;
#else
        return 0;
#endif
    }
}

void HttpClientRequest::setHttpMethodAndBody2(std::shared_ptr<FakeJni::JString> method, FakeJni::JLong callHandle, std::shared_ptr<FakeJni::JString> contentType, FakeJni::JLong contentLength) {
    this->method = method->asStdString();
    if (this->method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (this->method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (this->method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    } else if (this->method == "HEAD") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, this->method.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClientRequest::write_callback_wrapper);

    if(contentLength > 0) {
        this->inputStream = std::make_shared<NativeInputStream>(callHandle);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, this->inputStream.get());
        if (this->method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t) contentLength);
        } else if (this->method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) contentLength);
        }
#ifndef NDEBUG
        Log::trace("HttpClient", "setHttpMethodAndBody2 called, sent request");
#endif
    } else {
        if (this->method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
        }
#ifndef NDEBUG
        Log::trace("HttpClient", "setHttpMethodAndBody2 called, method: %s", this->method.c_str());
#endif
    }
    header = curl_slist_append(header, ("Content-Length: " + std::to_string(contentLength)).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    auto conttype = contentType->asStdString();
    if(conttype.length()) {
        header = curl_slist_append(header, ("Content-Type: " + conttype).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    }
}

void HttpClientRequest::setHttpHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value) {
#ifndef NDEBUG
    Log::trace("HttpClient", "setHttpHeader called, name: %s, value: %s", name->asStdString().c_str(), value->asStdString().c_str());
#endif
    header = curl_slist_append(header, (name->asStdString() + ": " + value->asStdString()).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
}

void HttpClientRequest::doRequestAsync(FakeJni::JLong sourceCall) {
    call_handle = sourceCall;
    auto me = this->shared_from_this();
    FakeJni::LocalFrame frame;
    auto&& jvm = &frame.getJniEnv().getVM();
    std::thread([=]() {
        FakeJni::JniEnvContext ctx(*jvm);
        try {
            auto anotherme = me;
            auto ret = curl_easy_perform(curl);
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            FakeJni::LocalFrame frame;
            if(ret == CURLE_OK) {
#ifndef NDEBUG
                Log::trace("HttpClient", "Response: code: %ld", response_code);
#endif
                auto method = getClass().getMethod("(JLcom/xbox/httpclient/HttpClientResponse;)V", "OnRequestCompleted");
                method->invoke(frame.getJniEnv(), this, sourceCall, frame.getJniEnv().createLocalReference(std::make_shared<HttpClientResponse>(sourceCall, response_code, response, headers)));
            } else {
                auto method = getClass().getMethod("(JLjava/lang/String;)V", "OnRequestFailed");
                method->invoke(frame.getJniEnv(), this, sourceCall, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>("Error")));
            }
        } catch(...) {
            FakeJni::LocalFrame frame(*jvm);
            auto method = getClass().getMethod("(JLjava/lang/String;)V", "OnRequestFailed");
            method->invoke(frame.getJniEnv(), this, sourceCall, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>("Error")));
        }
    }).detach();
}

FakeJni::JInt HttpClientResponse::getNumHeaders() {
    return headers.size();
}

std::shared_ptr<FakeJni::JString> HttpClientResponse::getHeaderNameAtIndex(FakeJni::JInt index) {
    return std::make_shared<FakeJni::JString>(headers[index].name);
}

std::shared_ptr<FakeJni::JString> HttpClientResponse::getHeaderValueAtIndex(FakeJni::JInt index) {
    return std::make_shared<FakeJni::JString>(headers[index].value);
}

std::shared_ptr<FakeJni::JByteArray> HttpClientResponse::getResponseBodyBytes() {
    return std::make_shared<FakeJni::JByteArray>(body);
}

void HttpClientResponse::getResponseBodyBytes2() {
    // Just in case setHttpMethodAndBody2 has been skipped and we have a response body
    if(!body.empty()) {
        std::make_shared<NativeOutputStream>(call_handle)->WriteAll(getResponseBodyBytes());
    }
}

FakeJni::JInt HttpClientResponse::getResponseCode() {
    return response_code;
}

HttpClientResponse::HttpClientResponse(FakeJni::JLong call_handle, int response_code, std::vector<signed char> body, std::vector<ResponseHeader> headers) : response_code(response_code),
                                                                                                                                                            body(body),
                                                                                                                                                            headers(headers),
                                                                                                                                                            call_handle(call_handle) {}

size_t HttpClientRequest::write_callback(char *ptr, size_t size, size_t nmemb) {
    try {
        auto byteArray = std::make_shared<FakeJni::JByteArray>(nmemb);
        memcpy(byteArray->getArray(), ptr, nmemb);
        std::make_shared<NativeOutputStream>(call_handle)->WriteAll(byteArray);
        return size * nmemb;
    } catch(...) {
#ifdef CURL_WRITEFUNC_ERROR
        return CURL_WRITEFUNC_ERROR;
#else
        return 0;
#endif
    }
}

// Unused in 1.18.30+, kept for compatibility with older versions
size_t HttpClientRequest::write_callback_old(char *ptr, size_t size, size_t nmemb) {
    response.insert(response.end(), ptr, ptr + nmemb);
    return size * nmemb;
}

size_t HttpClientRequest::header_callback(char *buffer, size_t size, size_t nitems) {
    auto string = std::string(buffer, nitems);
    auto location = string.find(": ");
    if(location != std::string::npos) {
        auto name = string.substr(0, location);
        auto value = string.substr(location + 1, string.length());
        trim(name);
        trim(value);
        headers.emplace_back(name, value);
    }
    return size * nitems;
}

NativeInputStream::NativeInputStream(FakeJni::JLong call_handle) : call_handle(call_handle) {
}

size_t NativeInputStream::Read(void *buffer, size_t size) {
    FakeJni::LocalFrame frame;
    auto method = getClass().getMethod("(JJ[BJJ)I", "nativeRead");
    auto buf = std::make_shared<FakeJni::JByteArray>((size_t)std::numeric_limits<jsize>::max() < size ? std::numeric_limits<jsize>::max() : (jsize)size);
    jvalue ret = method->invoke(frame.getJniEnv(), this, call_handle, offset, frame.getJniEnv().createLocalReference(buf), (FakeJni::JLong)0, (FakeJni::JLong)buf->getSize());
    if(ret.i != -1) {
        memcpy(buffer, buf->getArray(), ret.i);
        offset += ret.i;
    }
    return ret.i;
}

NativeOutputStream::NativeOutputStream(FakeJni::JLong call_handle) : call_handle(call_handle) {
}

void NativeOutputStream::WriteAll(std::shared_ptr<FakeJni::JByteArray> data) {
    FakeJni::LocalFrame frame;
    auto method = getClass().getMethod("(J[BII)V", "nativeWrite");
    method->invoke(frame.getJniEnv(), this, call_handle, frame.getJniEnv().createLocalReference(data), (FakeJni::JInt)0, (FakeJni::JInt)data->getSize());
}
