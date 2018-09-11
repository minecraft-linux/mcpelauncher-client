#pragma once

#include <functional>

class XboxLiveLoginInterface {

public:
    enum class ErrorCode {
        OK = 0,
        NoAccount = -1,
        ConnectionError = -2,
        InternalError = -3
    };

    virtual ~XboxLiveLoginInterface() {}

    virtual void invokeMsaAuthFlow(
            std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
            std::function<void(ErrorCode, std::string const&)> error_cb) = 0;

    virtual void invokeMsaRemoteAuthFlow(std::function<void (std::string const& code)> code_cb,
            std::function<void (std::string const& cid, std::string const& binaryToken)> success_cb,
            std::function<void (std::exception_ptr)> error_cb) {}

    virtual void requestXblToken(std::string const& cid, bool silent,
                                 std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                                 std::function<void(ErrorCode, std::string const&)> error_cb) = 0;

    virtual std::string getCllMsaToken(std::string const& cid) = 0;

    virtual bool isRemoteConnect() const { return false; }

};