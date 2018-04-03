#pragma once

class HTTPRequest;

class LinuxHttpRequestInternal {

public:
    int filler;
    HTTPRequest* request;

    LinuxHttpRequestInternal(HTTPRequest* request) {}

    virtual ~LinuxHttpRequestInternal() {}

    void send();

    void abort();
};

class LinuxHttpRequestHelper {

public:
    static void install(void* handle);

};