#include "msa_remote_login_task.h"

MsaRemoteLoginTask::~MsaRemoteLoginTask() {
    cancel();
    if (thread.joinable())
        thread.detach();
}

void MsaRemoteLoginTask::handleThread() {
    std::unique_lock<std::mutex> lock (mutex);
    try {
        MsaRemoteLogin login(clientId);
        MsaDeviceAuthConnectResponse deviceAuth = login.startDeviceAuthConnect(scope);
        if (codeCb)
            codeCb(deviceAuth.userCode);
    } catch (std::exception& e) {
        if (errorCb)
            errorCb(std::make_exception_ptr(e));
    }
}

void MsaRemoteLoginTask::start() {
    std::lock_guard<std::mutex> lock (mutex);
    if (started)
        throw std::logic_error("You cannot start a single login task more than once");
    started = true;
    thread = std::thread(&MsaRemoteLoginTask::handleThread, this);
}

void MsaRemoteLoginTask::cancel() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopping = true;
    }
    cv.notify_all();
}