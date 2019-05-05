#pragma once

#include <minecraft/std/string.h>
#include <minecraft/Xbox.h>

class XboxLivePatches {

private:
    static const char* TAG;

    static bool verifyCertChain();

    static mcpe::string readXboxConfigFile(void* th);

    static xbox::services::xbox_live_result<void> xboxLogTelemetrySignin(void* th, bool b, mcpe::string const& s);

    static mcpe::string getLocalStoragePath();

    static xbox::services::xbox_live_result<void>
    initSignInActivity(xbox::services::system::user_auth_android *th, int requestCode);

    static void invokeAuthFlow(void* auth);

    static std::vector<mcpe::string> getLocaleList();

    static void registerJavaInteropNatives();

    static xbox::services::xbox_live_result<void>
    logCLL(void *th, mcpe::string const &a, mcpe::string const &b, mcpe::string const &c);

    static bool useMinecraftVersionOfXBLUI();

    static void destroyXsapiSingleton(void* handle);

public:
    static void install(void* handle);

    static void workaroundShutdownFreeze(void* handle);

};