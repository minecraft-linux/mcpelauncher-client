#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include "msa_remote_login.h"

class MsaRemoteLoginTask {

private:
    const std::string clientId;
    const std::string scope;
    std::thread thread;

    std::mutex mutex;
    std::condition_variable cv;
    bool started = false;
    bool stopping = false;
    std::function<void (std::string const&)> codeCb;
    std::function<void (std::exception_ptr)> errorCb;

    void handleThread();

public:
    explicit MsaRemoteLoginTask(std::string clientId, std::string scope) : clientId(std::move(clientId)),
                                                                           scope(std::move(scope)) {}

    ~MsaRemoteLoginTask();

    void setCodeCallback(std::function<void (std::string const&)> callback) {
        std::lock_guard<std::mutex> lock (mutex);
        codeCb = std::move(callback);
    }

    void setErrorCallback(std::function<void (std::exception_ptr)> callback) {
        std::lock_guard<std::mutex> lock (mutex);
        errorCb = std::move(callback);
    }

    void start();

    void cancel();

};