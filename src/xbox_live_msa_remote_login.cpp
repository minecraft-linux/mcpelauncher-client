#include "xbox_live_msa_remote_login.h"

#include <minecraft/Xbox.h>
#include <log.h>

std::string const XboxLiveMsaRemoteLogin::MSA_CLIENT_ID = "00000000441cc96b";

XboxLiveMsaRemoteLogin::XboxLiveMsaRemoteLogin() {
}

void XboxLiveMsaRemoteLogin::invokeMsaAuthFlow(
        std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
        std::function<void(ErrorCode, std::string const&)> error_cb) {
    error_cb(ErrorCode::InternalError, "Unsupported operation");
}

void XboxLiveMsaRemoteLogin::requestXblToken(std::string const& cid, bool silent,
                                             std::function<void(std::string const& cid,
                                                                std::string const& binaryToken)> success_cb,
                                             std::function<void(ErrorCode, std::string const&)> error_cb) {
    error_cb(ErrorCode::InternalError, "Unsupported operation");
}

std::string XboxLiveMsaRemoteLogin::getCllMsaToken(std::string const& cid) {
    MsaRemoteLogin api (MSA_CLIENT_ID);
    std::string refreshToken = getRefreshToken();
    try {
        auto token = api.refreshToken(refreshToken, "service::vortex.data.microsoft.com::MBI_SSL");
        return token.accessToken;
    } catch (std::exception& e) {
        Log::warn("XboxLiveHelper", "Refreshing CLL token failed");
        return std::string();
    }
}

void XboxLiveMsaRemoteLogin::invokeMsaRemoteAuthFlow(std::function<void(std::string const& code)> code_cb,
                                                     std::function<void(std::string const& cid,
                                                                        std::string const& binaryToken)> success_cb,
                                                     std::function<void(std::exception_ptr)> error_cb) {
    if (msaRemoteLoginTask != nullptr)
        return;
    msaRemoteLoginTask = std::unique_ptr<MsaRemoteLoginTask>(
            new MsaRemoteLoginTask(MSA_CLIENT_ID, "service::user.auth.xboxlive.com::MBI_SSL"));
    msaRemoteLoginTask->setCodeCallback(std::move(code_cb));
    msaRemoteLoginTask->setSuccessCallback([success_cb](MsaAuthTokenResponse const& resp) {
        success_cb(resp.userId, "t=" + resp.accessToken);
    });
    msaRemoteLoginTask->setErrorCallback(std::move(error_cb));
    msaRemoteLoginTask->start();
}


std::string XboxLiveMsaRemoteLogin::getRefreshToken() {
    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    return local_conf->get_value_from_local_storage("oauth_refresh_token").std();
}

void XboxLiveMsaRemoteLogin::setRefreshToken(std::string const& token) {
    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    local_conf->write_value_to_local_storage("oauth_refresh_token", token);
}
