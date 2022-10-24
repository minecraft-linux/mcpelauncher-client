#include <FileUtil.h>
#include <mcpelauncher/path_helper.h>
#include <log.h>
#include "xbox_live.h"
#include "../xbox_live_helper.h"
#include <msa/client/error.h>

std::shared_ptr<FakeJni::JString> XboxInterop::getLocalStoragePath(std::shared_ptr<Context> context) {
    return std::make_shared<FakeJni::JString>(PathHelper::getPrimaryDataDirectory());
}

std::shared_ptr<FakeJni::JString> XboxInterop::readConfigFile(std::shared_ptr<Context> context) {
    std::string str;
    if(!FileUtil::readFile(PathHelper::findGameFile("assets/xboxservices.config"), str))
        str = "{}";
    return std::make_shared<FakeJni::JString>(str);
}

std::shared_ptr<FakeJni::JString> XboxInterop::getLocale() {
    return std::make_shared<FakeJni::JString>("en");
}

void XboxInterop::invokeMSA(std::shared_ptr<Context> context, FakeJni::JInt requestCode, FakeJni::JBoolean isProd,
                            std::shared_ptr<FakeJni::JString> cid) {
    Log::info("XboxInterop", "InvokeMSA: requestCode=%i cid=%s", requestCode, cid->asStdString().c_str());
    FakeJni::Jvm const *vm = &FakeJni::JniEnv::getCurrentEnv()->getVM();

    if(requestCode == 1) {  // Silent sign in
        XboxLiveHelper::getInstance().requestXblToken(
            cid->asStdString(), true,
            [vm, requestCode](std::string const &cid, std::string const &binaryToken) {
                ticketCallback(*vm, binaryToken, requestCode, TICKET_OK, "");
            },
            [vm, requestCode](simpleipc::rpc_error_code code, std::string const &err) {
                if(code == msa::client::ErrorCodes::NoSuchAccount)
                    ticketCallback(*vm, "", requestCode, TICKET_UI_INTERACTION_REQUIRED, "Must show UI to acquire an account.");
                else if(code == msa::client::ErrorCodes::MustShowUI)
                    ticketCallback(*vm, "", requestCode, TICKET_UI_INTERACTION_REQUIRED, "Must show UI to update account information.");
                else
                    ticketCallback(*vm, "", requestCode, TICKET_UNKNOWN_ERROR, err);
            });
    } else if(requestCode == 6) {  // Sign out
        signOutCallback();
    } else {
        throw std::runtime_error("Unsupported requestCode");
    }
}

void XboxInterop::invokeAuthFlow(FakeJni::JLong userPtr, std::shared_ptr<Activity> activity, FakeJni::JBoolean isProd,
                                 std::shared_ptr<FakeJni::JString> signInText) {
    Log::info("XboxInterop", "InvokeAuthFlow");
    FakeJni::Jvm const *vm = &FakeJni::JniEnv::getCurrentEnv()->getVM();

    XboxLiveHelper::getInstance().invokeMsaAuthFlow([vm, userPtr](std::string const &cid, std::string const &binaryToken) {
        auto cb = std::make_shared<XboxLoginCallback>(*vm, userPtr, cid, binaryToken);
        invokeXBLogin(*vm, userPtr, binaryToken, cb); }, [vm, userPtr](simpleipc::rpc_error_code c, std::string const &) {
        if (c == msa::client::ErrorCodes::OperationCancelled)
            authFlowCallback(*vm, userPtr, AUTH_FLOW_CANCEL, "");
        else
            authFlowCallback(*vm, userPtr, AUTH_FLOW_ERROR, ""); });
}

void XboxInterop::initCLL(std::shared_ptr<Context> arg0, std::shared_ptr<FakeJni::JString> arg1) {
    XboxLiveHelper::getInstance().initCll();
}

void XboxInterop::logCLL(std::shared_ptr<FakeJni::JString> ticket, std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> data) {
    cll::Event event(name->asStdString(), nlohmann::json::parse(data->asStdString()),
                     cll::EventFlags::PersistenceCritical | cll::EventFlags::LatencyRealtime, {ticket->asStdString()});
    XboxLiveHelper::getInstance().logCll(event);
}

void XboxInterop::ticketCallback(FakeJni::Jvm const &vm, std::string const &ticket, int requestCode, int errorCode,
                                 std::string const &error) {
    FakeJni::LocalFrame env(vm);
    auto callback = getDescriptor()->getMethod("(Ljava/lang/String;IILjava/lang/String;)V", "ticket_callback");
    auto ticketRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(ticket));
    auto errorStrRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(error));
    callback->invoke(env.getJniEnv(), getDescriptor().get(), ticketRef, requestCode, errorCode, errorStrRef);
}

void XboxInterop::authFlowCallback(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, int status, std::string const &cid) {
    FakeJni::LocalFrame env(vm);
    auto callback = getDescriptor()->getMethod("(JILjava/lang/String;)V", "auth_flow_callback");
    auto cidRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(cid));
    callback->invoke(env.getJniEnv(), getDescriptor().get(), userPtr, status, cidRef);
}

void XboxInterop::signOutCallback() {
    FakeJni::LocalFrame env;
    auto callback = getDescriptor()->getMethod("()V", "sign_out_callback");
    callback->invoke(env.getJniEnv(), getDescriptor().get());
}

void XboxInterop::invokeXBLogin(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, std::string const &ticket,
                                std::shared_ptr<XboxLoginCallback> callback) {
    FakeJni::LocalFrame env(vm);
    auto fn = getDescriptor()->getMethod("(JLjava/lang/String;Lcom/microsoft/xbox/idp/interop/Interop$XBLoginCallback;)V", "invoke_xb_login");
    auto ticketRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(ticket));
    auto callbackRef = env.getJniEnv().createLocalReference(callback);
    fn->invoke(env.getJniEnv(), getDescriptor().get(), userPtr, ticketRef, callbackRef);
}

void XboxInterop::invokeEventInitialization(FakeJni::Jvm const &vm, FakeJni::JLong userPtr, std::string const &ticket,
                                            std::shared_ptr<XboxLoginCallback> callback) {
    FakeJni::LocalFrame env(vm);
    auto fn = getDescriptor()->getMethod("(JLjava/lang/String;Lcom/microsoft/xbox/idp/interop/Interop$EventInitializationCallback;)V", "invoke_event_initialization");
    auto ticketRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(ticket));
    auto callbackRef = env.getJniEnv().createLocalReference(callback);
    fn->invoke(env.getJniEnv(), getDescriptor().get(), userPtr, ticketRef, callbackRef);
}

void XboxLoginCallback::onLogin(FakeJni::JLong nativePtr, FakeJni::JBoolean newAccount) {
    XboxInterop::invokeEventInitialization(jvm, userPtr, ticket,
                                           std::static_pointer_cast<XboxLoginCallback>(shared_from_this()));
}

void XboxLoginCallback::onSuccess() {
    XboxInterop::authFlowCallback(jvm, userPtr, XboxInterop::AUTH_FLOW_OK, cid);
}

void XboxLoginCallback::onError(int httpStatus, int status, std::shared_ptr<FakeJni::JString> message) {
    XboxInterop::authFlowCallback(jvm, userPtr, XboxInterop::AUTH_FLOW_ERROR, "");
}

std::shared_ptr<FakeJni::JString> XboxLocalStorage::getPath(std::shared_ptr<Context> context) {
    return XboxInterop::getLocalStoragePath(context);
}
