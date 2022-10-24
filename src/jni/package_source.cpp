#include "package_source.h"

NativePackageSourceListener::NativePackageSourceListener() {
}

std::shared_ptr<PackageSource> PackageSourceFactory::createGooglePlayPackageSource(std::shared_ptr<FakeJni::JString> a, std::shared_ptr<PackageSourceListener> l) {
    return std::make_shared<PackageSource>();
}
