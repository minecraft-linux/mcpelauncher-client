#pragma once

#include <minecraft/std/string.h>
#include <minecraft/Xbox.h>
#include <minecraft/MinecraftScreenModel.h>

namespace web { namespace json { class value; } }

class XboxLivePatches {

private:
    static const char* TAG;

    static bool verifyCertChain();

    static mcpe::string readXboxConfigFile(void* th);

    static xbox::services::xbox_live_result<void> xboxLogTelemetrySignin(void* th, bool b, mcpe::string const& s);

    static mcpe::string getLocalStoragePath();

    static xbox::services::xbox_live_result<void>
    initSignInActivity(xbox::services::system::user_auth_android *th, int requestCode);

    static void invokeAuthFlow(xbox::services::system::user_auth_android* auth);

    static std::vector<mcpe::string> getLocaleList();

    static void registerJavaInteropNatives();

    static xbox::services::xbox_live_result<void>
    logCLL(void *th, mcpe::string const &a, mcpe::string const &b, mcpe::string const &c);

    static bool useMinecraftVersionOfXBLUI();

    static void destroyXsapiSingleton(void* handle);

    static web::json::value (*createDeviceTokenRequestOriginal)(mcpe::string, mcpe::string, void*, mcpe::string,
                                                                mcpe::string, mcpe::string);
    static web::json::value createDeviceTokenRequestHook(mcpe::string a, mcpe::string b, void* c, mcpe::string d,
                                                         mcpe::string e, mcpe::string f);

    static void (*signInOriginal)(MinecraftScreenModel*, mcpe::function<void ()>,
                                  mcpe::function<void (Social::SignInResult, bool)>);

    static void signInHook(MinecraftScreenModel* th, mcpe::function<void ()> cancelCb,
                           mcpe::function<void (Social::SignInResult, bool)> cb);

public:
    static void install(void* handle);

    static void workaroundShutdownFreeze(void* handle);

};