#pragma once

#include <mutex>
#include <condition_variable>

class XboxSleepShutdownPatch {

private:
    static std::condition_variable cv;
    static std::mutex mutex;
    static bool shuttingDown;

    static void sleepHook(unsigned int ms);

public:
    static void install(void* handle);

    static void notifyShutdown();

};