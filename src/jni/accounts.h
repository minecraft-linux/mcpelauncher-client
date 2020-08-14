#pragma once

#include <fake-jni/jvm.h>
#include "main_activity.h"

class Account : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/accounts/Account")
};

class AccountManager : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/accounts/AccountManager")
    static std::shared_ptr<AccountManager> get(std::shared_ptr<Context>);
    std::shared_ptr<FakeJni::JArray<Account>> getAccountsByType(std::shared_ptr<FakeJni::JString>);
};
