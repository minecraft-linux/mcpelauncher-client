#pragma once

#include <mutex>
#include <condition_variable>

class XboxSleepShutdownPatch {

private:
    static std::condition_variable cv;
    static std::mutex mutex;
    static std::mutex ensureConnectedMutex;
    static bool shuttingDown;

    static void (*ensureConnectedOriginal)(void*);
    static void ensureConnectedHook(void*);

    static void sleepHook(unsigned int ms);

    static bool tryInstallWSPatch(void* handle);

public:
    static void install(void* handle);

    static void notifyShutdown();

};