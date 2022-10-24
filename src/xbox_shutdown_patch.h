#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

class XboxShutdownPatch {
private:
    static std::condition_variable cv;
    static std::mutex mutex;
    static bool shuttingDown;

    static void sleepHook(unsigned int ms);

public:
    static std::atomic_int runningTasks;
    static std::mutex runningTasksMutex;
    static std::condition_variable runningTasksCv;

    static void install(void* handle);

    static void notifyShutdown();
};
