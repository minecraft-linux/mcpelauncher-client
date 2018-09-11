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
struct MsaAuthTokenResponse {
    std::string userId;
    std::string tokenType;
    std::string scope;
    std::string accessToken;
    std::string refreshToken;
    int expiresIn;
};
struct MsaDeviceAuthPollResponse : public MsaAuthTokenResponse {
    bool userNotSignedInYet = false;
};

class MsaRemoteLogin {

private:
    const std::string clientId;

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
    explicit MsaRemoteLogin(std::string clientId) : clientId(std::move(clientId)) {}

    MsaDeviceAuthConnectResponse startDeviceAuthConnect(std::string const& scope);

    MsaDeviceAuthPollResponse pollDeviceAuthState(std::string const& deviceCode);

    MsaDeviceAuthPollResponse refreshToken(std::string const& token, std::string const& scope);

};
