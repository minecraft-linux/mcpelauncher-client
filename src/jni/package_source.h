#pragma once

#include <fake-jni/fake-jni.h>
#include "java_types.h"

class PackageSource : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/packagesource/PackageSource")
};

class PackageSourceListener : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/packagesource/PackageSourceListener")
};

class NativePackageSourceListener : public PackageSourceListener {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/packagesource/NativePackageSourceListener", PackageSourceListener)
    NativePackageSourceListener();
};

class PackageSourceFactory : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/packagesource/PackageSourceFactory")

    static std::shared_ptr<PackageSource> createGooglePlayPackageSource(std::shared_ptr<FakeJni::JString>, std::shared_ptr<PackageSourceListener>);
};
