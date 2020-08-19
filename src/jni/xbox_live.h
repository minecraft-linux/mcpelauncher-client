#pragma once

#include <fake-jni/fake-jni.h>
#include "main_activity.h"
#include <cstring>
#include <openssl/evp.h>
#include <iostream>

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

    XboxLoginCallback(FakeJni::Jvm const &jvm, FakeJni::JLong userPtr, std::string cid, std::string ticket) :
        jvm(jvm), userPtr(userPtr), cid(std::move(cid)), ticket(std::move(ticket)) {}

    void onLogin(FakeJni::JLong nativePtr, FakeJni::JBoolean newAccount);

    void onSuccess();

    void onError(int httpStatus, int status, std::shared_ptr<FakeJni::JString> message);

};

class ShaHasher : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/ShaHasher")
    EVP_MD_CTX *mdctx = NULL;
    
    ShaHasher() {
        const EVP_MD* md = EVP_get_digestbynid(NID_sha256);
        if (md == NULL) {
            throw std::runtime_error("OpenSSL failed to get \"SHA2-256\"");
        }
        mdctx = EVP_MD_CTX_new();
        if (md == NULL) {
            throw std::runtime_error("OpenSSL failed to create mdctx");
        }
        if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
            throw std::runtime_error("OpenSSL failed to initialize evp digest");
        }
    }
    
    ~ShaHasher() {
        EVP_MD_CTX_free(mdctx);
    }

    void AddBytes(std::shared_ptr<FakeJni::JByteArray> barray) {
        auto data = barray->getArray();
        auto len = barray->getSize();
        if (EVP_DigestUpdate(mdctx, data, len) != 1) {
           throw std::runtime_error("OpenSSL failed to update evp digest");
        }
    }

    std::shared_ptr<FakeJni::JByteArray> SignHash() {
        unsigned char md_value[EVP_MAX_MD_SIZE];
        unsigned int md_len = 0;
        if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
           throw std::runtime_error("OpenSSL failed to finish evp digest");
        }
        auto arr = std::make_shared<FakeJni::JByteArray>(md_len);
        memcpy(arr->getArray(), md_value, md_len);
        return arr;
    }
};

class SecureRandom : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/SecureRandom")
    static std::shared_ptr<FakeJni::JByteArray> GenerateRandomBytes(int bytes) {
        return std::make_shared<FakeJni::JByteArray>(bytes);
    }
};

class WebView : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/browser/WebView")

    static void showUrl(FakeJni::JLong l, std::shared_ptr<Context> ctx, std::shared_ptr<FakeJni::JString> starturl, std::shared_ptr<FakeJni::JString> endurl, FakeJni::JInt i, FakeJni::JBoolean z, FakeJni::JLong j2) {
        auto a = starturl->asStdString();
        auto b = endurl->asStdString();
        system((
#if defined(__APPLE__)
            "open "
#elif defined (_WIN32)
            "start "
#else
            "xdg-open "
#endif
  + a).c_str());
        std::cout << "OpenThisURL: " << a << "\n";
        std::string result = "";
        std::getline(std::cin, result);
        auto method = WebView::getDescriptor()->getMethod("(JLjava/lang/String;ZLjava/lang/String;)V", "urlOperationSucceeded");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), WebView::getDescriptor().get(), l, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(result.data())), false, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>("webkit-noDefault::0::none")));
    }
};
