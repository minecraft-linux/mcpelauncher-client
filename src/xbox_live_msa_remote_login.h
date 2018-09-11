#pragma once

#include "xbox_live_login_interface.h"
#include "msa_remote_login_task.h"

class XboxLiveMsaRemoteLogin : public XboxLiveLoginInterface {

private:
    static std::string const MSA_CLIENT_ID;
    static std::string const XBOX_LIVE_SCOPE;
    static std::string const VORTEX_SCOPE;

    std::unique_ptr<MsaRemoteLoginTask> msaRemoteLoginTask;

public:
    XboxLiveMsaRemoteLogin();

    void invokeMsaAuthFlow(std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                           std::function<void(ErrorCode, std::string const&)> error_cb) override;

    void requestXblToken(std::string const& cid, bool silent,
                         std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                         std::function<void(ErrorCode, std::string const&)> error_cb) override;

    std::string getCllMsaToken(std::string const& cid) override;


    void invokeMsaRemoteAuthFlow(std::function<void (std::string const& code)> code_cb,
                                 std::function<void (std::string const& cid,
                                         std::string const& binaryToken)> success_cb,
                                 std::function<void (std::exception_ptr)> error_cb) override;

    void cancelMsaRemoteAuthFlow() override;

    bool isRemoteConnect() const override { return true; }


    static std::string getCID();

    static std::string getRefreshToken();

    static void setRefreshToken(std::string const& token);

};