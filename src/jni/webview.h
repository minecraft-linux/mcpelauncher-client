#pragma once
#include <fake-jni/fake-jni.h>
#include "main_activity.h"

class WebView : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/browser/WebView")

    static void showUrl(FakeJni::JLong l, std::shared_ptr<Context> ctx, std::shared_ptr<FakeJni::JString> starturl, std::shared_ptr<FakeJni::JString> endurl, FakeJni::JInt i, FakeJni::JBoolean z, FakeJni::JLong j2);
};

class BrowserLaunchActivity : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/browser/BrowserLaunchActivity")

    static void showUrl(FakeJni::JLong l, std::shared_ptr<Context> ctx, std::shared_ptr<FakeJni::JString> starturl, std::shared_ptr<FakeJni::JString> endurl, FakeJni::JInt i, std::shared_ptr<FakeJni::JArray<FakeJni::JString>>, std::shared_ptr<FakeJni::JArray<FakeJni::JString>>, FakeJni::JBoolean z, FakeJni::JLong j2);
    static void showUrl2(FakeJni::JLong l, std::shared_ptr<Context> ctx, std::shared_ptr<FakeJni::JString> starturl, std::shared_ptr<FakeJni::JString> endurl, FakeJni::JInt i, std::shared_ptr<FakeJni::JArray<FakeJni::JString>>, std::shared_ptr<FakeJni::JArray<FakeJni::JString>>, FakeJni::JBoolean z);
};
