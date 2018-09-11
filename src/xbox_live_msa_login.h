#pragma once

#include <msa/client/service_launcher.h>
#include <msa/client/service_client.h>
#include "xbox_live_login_interface.h"

class XboxLiveMsaLogin : public XboxLiveLoginInterface {

private:
    static std::string const MSA_CLIENT_ID;
    static std::string const MSA_COBRAND_ID;

    msa::client::ServiceLauncher launcher;
    std::unique_ptr<msa::client::ServiceClient> client;

    static std::string findMsa();

    static ErrorCode translateErrorCode(simpleipc::rpc_error_code code);

public:
    XboxLiveMsaLogin();

    void invokeMsaAuthFlow(std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                           std::function<void(ErrorCode, std::string const&)> error_cb) override;

    void requestXblToken(std::string const& cid, bool silent,
                         std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                         std::function<void(ErrorCode, std::string const&)> error_cb) override;

    simpleipc::client::rpc_call<std::shared_ptr<msa::client::Token>> requestXblToken(std::string const& cid,
                                                                                     bool silent);

    std::string getCllMsaToken(std::string const& cid) override;

};