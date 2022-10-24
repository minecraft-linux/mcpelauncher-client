#include "cert_manager.h"

ByteArrayInputStream::ByteArrayInputStream(std::shared_ptr<FakeJni::JByteArray> bytes) {
}

std::shared_ptr<CertificateFactory> CertificateFactory::getInstance(std::shared_ptr<FakeJni::JString> s) {
    return std::make_shared<CertificateFactory>();
}

std::shared_ptr<Certificate> CertificateFactory::generateCertificate(std::shared_ptr<InputStream> s) {
    return std::make_shared<X509Certificate>();
}

std::shared_ptr<TrustManagerFactory> TrustManagerFactory::getInstance(std::shared_ptr<FakeJni::JString> s) {
    return std::make_shared<TrustManagerFactory>();
}

std::shared_ptr<FakeJni::JArray<TrustManager>> TrustManagerFactory::getTrustManagers() {
    auto a = std::make_shared<FakeJni::JArray<TrustManager>>(1);
    (*a)[0] = std::make_shared<X509TrustManager>();
    return a;
}

StrictHostnameVerifier::StrictHostnameVerifier() {
}

void StrictHostnameVerifier::verify(std::shared_ptr<FakeJni::JString> s, std::shared_ptr<X509Certificate> cert) {
}
