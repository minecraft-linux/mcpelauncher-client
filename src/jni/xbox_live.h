#pragma once

#include <fake-jni/fake-jni.h>
#include "main_activity.h"

class XboxLoginCallback;

class XboxInterop : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xbox/idp/interop/Interop")

    static std::shared_ptr<FakeJni::JString> getLocalStoragePath(std::shared_ptr<Context> context);

    static std::shared_ptr<FakeJni::JString> readConfigFile(std::shared_ptr<Context> context);

    static std::shared_ptr<FakeJni::JString> getLocale();

    static void invokeMSA(std::shared_ptr<Context> context, FakeJni::JInt requestCode, FakeJni::JBoolean isProd,
                          std::shared_ptr<FakeJni::JString> cid);

    static void invokeAuthFlow(FakeJni::JLong userPtr, std::shared_ptr<Activity> activity, FakeJni::JBoolean isProd,
                               std::shared_ptr<FakeJni::JString> signInText);

    static void initCLL(std::shared_ptr<Context> arg0, std::shared_ptr<FakeJni::JString> arg1);

    static void logCLL(std::shared_ptr<FakeJni::JString> ticket, std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> data);

private:
    friend class XboxLoginCallback;

    static constexpr int AUTH_FLOW_OK = 0;
    static constexpr int AUTH_FLOW_CANCEL = 1;
    static constexpr int AUTH_FLOW_ERROR = 2;

    static constexpr int TICKET_OK = 0;
    static constexpr int TICKET_UI_INTERACTION_REQUIRED = 1;
    static constexpr int TICKET_UNKNOWN_ERROR = 3;

    static void ticketCallback(FakeJni::Jvm const &vm, std::string const &ticket, int requestCode, int errorCode,
                               std::string const &errorStr);

    static void authFlowCallback(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, int status, std::string const &cid);

    static void signOutCallback();

    static void invokeXBLogin(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, std::string const &ticket,
                              std::shared_ptr<XboxLoginCallback> callback);

    static void invokeEventInitialization(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, std::string const &ticket,
                                          std::shared_ptr<XboxLoginCallback> callback);
};

class XboxLoginCallback : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xbox/idp/interop/Interop$XBLoginCallback")

    FakeJni::Jvm const &jvm;
    FakeJni::JLong userPtr;
    std::string cid, ticket;

    XboxLoginCallback(FakeJni::Jvm const &jvm, FakeJni::JLong userPtr, std::string cid, std::string ticket) : jvm(jvm), userPtr(userPtr), cid(std::move(cid)), ticket(std::move(ticket)) {}

    void onLogin(FakeJni::JLong nativePtr, FakeJni::JBoolean newAccount);

    void onSuccess();

    void onError(int httpStatus, int status, std::shared_ptr<FakeJni::JString> message);
};

class XboxLocalStorage : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xboxlive/LocalStorage")

    static std::shared_ptr<FakeJni::JString> getPath(std::shared_ptr<Context> context);
};
