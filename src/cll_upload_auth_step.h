#pragma once

#include <mutex>
#include <cll/event_upload_step.h>

class CllUploadAuthStep : public cll::EventUploadStep {
private:
    std::recursive_mutex mutex;
    std::recursive_mutex cidMutex;
    std::string cid;
    std::string tokensCid;
    std::string msaToken;
    std::string xToken;

    void refreshTokens(bool force = false);

public:
    void setAccount(std::string const& cid);

    void onRequest(cll::EventUploadRequest& request) override;

    bool onAuthenticationFailed() override;
};
