#include "xbox_live_msa_login.h"

#include <log.h>
#include <msa/client/compact_token.h>

using namespace simpleipc;

std::string const XboxLiveMsaLogin::MSA_CLIENT_ID = "android-app://com.mojang.minecraftpe.H62DKCBHJP6WXXIV7RBFOGOL4NAK4E6Y";
std::string const XboxLiveMsaLogin::MSA_COBRAND_ID = "90023";

XboxLiveMsaLogin::XboxLiveMsaLogin() : launcher(findMsa()) {
    client = std::unique_ptr<msa::client::ServiceClient>(new msa::client::ServiceClient(launcher));
}

std::string XboxLiveMsaLogin::findMsa() {
    std::string path;
#ifdef MSA_DAEMON_PATH
    if (EnvPathUtil::findInPath("msa-daemon", path, MSA_DAEMON_PATH, EnvPathUtil::getAppDir().c_str()))
        return path;
#endif
    if (EnvPathUtil::findInPath("msa-daemon", path))
        return path;
    return std::string();
}

void XboxLiveMsaLogin::invokeMsaAuthFlow(
        std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
        std::function<void(ErrorCode, std::string const&)> error_cb) {
    if (!client) {
        error_cb(ErrorCode::ConnectionError, "Could not connect to the daemon");
        return;
    }
    client->pickAccount(MSA_CLIENT_ID, MSA_COBRAND_ID).call([this, success_cb, error_cb](rpc_result<std::string> res) {
        if (!res.success()) {
            error_cb(translateErrorCode(res.error_code()), res.error_text());
            return;
        }

        requestXblToken(res.data(), false, success_cb, error_cb);
    });
}

void XboxLiveMsaLogin::requestXblToken(std::string const& cid, bool silent,
                                       std::function<void(std::string const& cid,
                                                          std::string const& binaryToken)> success_cb,
                                       std::function<void(ErrorCode, std::string const&)> error_cb) {
    if (!client) {
        error_cb(ErrorCode::ConnectionError, "Could not connect to the daemon");
        return;
    }
    requestXblToken(cid, silent).call([cid, success_cb, error_cb](rpc_result<std::shared_ptr<msa::client::Token>> res) {
        if (res.success() && res.data() && res.data()->getType() == msa::client::TokenType::Compact) {
            auto token = msa::client::token_pointer_cast<msa::client::CompactToken>(res.data());
            success_cb(cid, token->getBinaryToken());
        } else {
            if (res.success())
                error_cb(ErrorCode::InternalError, "Invalid token received from the MSA daemon");
            else
                error_cb(translateErrorCode(res.error_code()), res.error_text());
        }
    });
}

simpleipc::client::rpc_call<std::shared_ptr<msa::client::Token>> XboxLiveMsaLogin::requestXblToken(
        std::string const& cid, bool silent) {
    if (!client)
        throw std::runtime_error("Could not connect to the daemon");
    return client->requestToken(cid, {"user.auth.xboxlive.com", "mbi_ssl"}, MSA_CLIENT_ID, silent);
}

std::string XboxLiveMsaLogin::getCllMsaToken(std::string const& cid) {
    if (!client)
        return std::string();
    auto token = client->requestToken(cid, {"vortex.data.microsoft.com", "mbi_ssl"}, MSA_CLIENT_ID, true).call();
    if (!token.success() || !token.data() || token.data()->getType() != msa::client::TokenType::Compact)
        return std::string();
    return msa::client::token_pointer_cast<msa::client::CompactToken>(token.data())->getBinaryToken();
}

XboxLiveLoginInterface::ErrorCode XboxLiveMsaLogin::translateErrorCode(simpleipc::rpc_error_code code) {
    if (code == simpleipc::rpc_error_codes::success)
        return ErrorCode::OK;
    if (code == simpleipc::rpc_error_codes::connection_closed)
        return ErrorCode::ConnectionError;
    if (code == -100)
        return ErrorCode::NoAccount;
    return ErrorCode::OK;
}