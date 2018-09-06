#pragma once

#include <string>
#include <vector>

struct MsaDeviceAuthConnectResponse {
    std::string userCode;
    std::string deviceCode;
    std::string verificationUri;
    int interval;
    int expiresIn;
};

class MsaRemoteLogin {

private:
    std::string clientId;

    struct Request {
        std::string url;
        std::vector<std::pair<std::string, std::string>> postData;

        Request(std::string url) : url(std::move(url)) {}
    };
    struct Response {
        long status;
        std::string body;
    };

    Response send(Request const& req);

public:
    MsaRemoteLogin(std::string clientId) : clientId(std::move(clientId)) {}

    MsaDeviceAuthConnectResponse startDeviceAuthConnect(std::string const& scope);

};