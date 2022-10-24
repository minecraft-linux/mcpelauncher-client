#include <log.h>
#include "accounts.h"

std::shared_ptr<AccountManager> AccountManager::get(std::shared_ptr<Context>) {
    return std::make_shared<AccountManager>(AccountManager());
}

std::shared_ptr<FakeJni::JArray<Account>> AccountManager::getAccountsByType(std::shared_ptr<FakeJni::JString> type) {
    // Log::info("Account", type->asStdString().c_str());
    // return std::make_shared<FakeJni::JArray<Account>, std::initializer_list<std::shared_ptr<Account>>>({std::make_shared<Account>()});
    return std::make_shared<FakeJni::JArray<Account>>(0);
}
