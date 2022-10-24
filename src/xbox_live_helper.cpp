#include <msa/client/compact_token.h>
#include <log.h>
#include <FileUtil.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/minecraft_version.h>
#include "xbox_live_helper.h"
#include "jni/xbox_live.h"

using namespace simpleipc;

XboxLiveHelper XboxLiveHelper::instance;

std::string const XboxLiveHelper::MSA_CLIENT_ID = "android-app://com.mojang.minecraftpe.H62DKCBHJP6WXXIV7RBFOGOL4NAK4E6Y";
std::string const XboxLiveHelper::MSA_COBRAND_ID = "90023";

std::string XboxLiveHelper::findMsa() {
    std::string path;
#ifdef MSA_DAEMON_PATH
    if(EnvPathUtil::findInPath("msa-daemon", path, MSA_DAEMON_PATH, EnvPathUtil::getAppDir().c_str()))
        return path;
#endif
    if(EnvPathUtil::findInPath("msa-daemon", path))
        return path;
    return std::string();
}

XboxLiveHelper::XboxLiveHelper() : launcher(findMsa()), triedToCreateClient(false) {
}

msa::client::ServiceClient* XboxLiveHelper::getMsaClientOrNull() {
    std::lock_guard<std::mutex> lock(clientMutex);
    if(triedToCreateClient)
        return client.get();
    triedToCreateClient = true;
    try {
        client = std::unique_ptr<msa::client::ServiceClient>(new msa::client::ServiceClient(launcher));
    } catch(std::exception& exception) {
        Log::error("XboxLiveHelper", "Failed to connect to the daemon: %s", exception.what());
    }
    return client.get();
}

msa::client::ServiceClient& XboxLiveHelper::getMsaClient() {
    auto client = getMsaClientOrNull();
    if(!client)
        throw std::runtime_error("Could not connect to the daemon");
    return *client;
}

void XboxLiveHelper::invokeMsaAuthFlow(
    std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
    std::function<void(simpleipc::rpc_error_code, std::string const&)> error_cb) {
    auto client = getMsaClientOrNull();
    if(!client) {
        error_cb(simpleipc::rpc_error_codes::connection_closed, "Could not connect to the daemon");
        return;
    }
    client->pickAccount(MSA_CLIENT_ID, MSA_COBRAND_ID).call([this, success_cb, error_cb](rpc_result<std::string> res) {
        if(!res.success()) {
            error_cb(res.error_code(), res.error_text());
            return;
        }

        requestXblToken(res.data(), false, success_cb, error_cb);
    });
}

simpleipc::client::rpc_call<std::shared_ptr<msa::client::Token>> XboxLiveHelper::requestXblToken(std::string const& cid,
                                                                                                 bool silent) {
    return getMsaClient().requestToken(cid, {"user.auth.xboxlive.com", "mbi_ssl"}, MSA_CLIENT_ID, silent);
}

void XboxLiveHelper::requestXblToken(std::string const& cid, bool silent,
                                     std::function<void(std::string const& cid, std::string const& binaryToken)> success_cb,
                                     std::function<void(simpleipc::rpc_error_code, std::string const&)> error_cb) {
    if(!getMsaClientOrNull()) {
        error_cb(simpleipc::rpc_error_codes::connection_closed, "Could not connect to the daemon");
        return;
    }
    requestXblToken(cid, silent).call([cid, success_cb, error_cb](rpc_result<std::shared_ptr<msa::client::Token>> res) {
        if(res.success() && res.data() && res.data()->getType() == msa::client::TokenType::Compact) {
            auto token = msa::client::token_pointer_cast<msa::client::CompactToken>(res.data());
            success_cb(cid, token->getBinaryToken());
        } else {
            if(res.success())
                error_cb(simpleipc::rpc_error_codes::internal_error, "Invalid token received from the MSA daemon");
            else
                error_cb(res.error_code(), res.error_text());
        }
    });
}

void XboxLiveHelper::initCll(std::string const& cid) {
    std::lock_guard<std::mutex> lock(cllMutex);
    if(!cid.empty())
        cllAuthStep.setAccount(cid);
    if(cll)
        return;
    std::string iKey = "P-XBL-T" + std::to_string(0);  // TODO: Get tid properly
    auto cllEvents = PathHelper::getPrimaryDataDirectory() + "cll_events";
    auto cacheDir = PathHelper::getCacheDirectory() + "cll";
    FileUtil::mkdirRecursive(cllEvents);
    FileUtil::mkdirRecursive(cacheDir);
    cll = std::unique_ptr<cll::EventManager>(new cll::EventManager(iKey, cllEvents, cacheDir));
    cll->addUploadStep(cllAuthStep);
    cll->setApp("A:com.mojang.minecraftpe", MinecraftVersion::getString());
    cll->start();
}

std::string XboxLiveHelper::getCllMsaToken(std::string const& cid) {
    auto client = getMsaClientOrNull();
    if(!client)
        return std::string();
    auto token = client->requestToken(cid, {"vortex.data.microsoft.com", "mbi_ssl"}, MSA_CLIENT_ID, true).call();
    if(!token.success() || !token.data() || token.data()->getType() != msa::client::TokenType::Compact)
        return std::string();
    return msa::client::token_pointer_cast<msa::client::CompactToken>(token.data())->getBinaryToken();
}

void XboxLiveHelper::setJvm(FakeJni::Jvm* vm) {
    this->vm = vm;
}

std::string XboxLiveHelper::getCllXToken(bool refresh) {
    FakeJni::LocalFrame env(*vm);
    if(auto callback = XboxInterop::getDescriptor()->getMethod("(Z)Ljava/lang/String;", "get_uploader_x_token_callback")) {
        if(auto xtokenRef = callback->invoke(env.getJniEnv(), XboxInterop::getDescriptor().get(), (jboolean)refresh).l) {
            if(auto xtoken = std::dynamic_pointer_cast<FakeJni::JString>(env.getJniEnv().resolveReference(xtokenRef))) {
                return xtoken->asStdString();
            }
        }
    }
    return std::string();
}

std::string XboxLiveHelper::getCllXTicket(std::string const& xuid) {
    FakeJni::LocalFrame env(*vm);
    if(auto callback = XboxInterop::getDescriptor()->getMethod("(Ljava/lang/String;)Ljava/lang/String;", "get_supporting_x_token_callback")) {
        auto xuidRef = env.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(xuid));
        if(auto xticketRef = callback->invoke(env.getJniEnv(), XboxInterop::getDescriptor().get(), xuidRef).l) {
            if(auto xticket = std::dynamic_pointer_cast<FakeJni::JString>(env.getJniEnv().resolveReference(xticketRef))) {
                return xticket->asStdString();
            }
        }
    }
    return std::string();
}

void XboxLiveHelper::logCll(cll::Event const& event) {
    initCll();
    cll->add(event);
}
