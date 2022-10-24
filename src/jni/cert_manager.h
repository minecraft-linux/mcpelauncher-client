#pragma once

#include <fake-jni/fake-jni.h>
#include "java_types.h"
#include "../text_input_handler.h"

class InputStream : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/io/InputStream")
};

class ByteArrayInputStream : public InputStream {
public:
    DEFINE_CLASS_NAME("java/io/ByteArrayInputStream", InputStream)
    ByteArrayInputStream(std::shared_ptr<FakeJni::JByteArray> bytes);
};

class Certificate : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/security/cert/Certificate")
};

class X509Certificate : public Certificate {
public:
    DEFINE_CLASS_NAME("org/apache/http/conn/ssl/java/security/cert/X509Certificate", Certificate)
};

class CertificateFactory : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/security/cert/CertificateFactory")
    static std::shared_ptr<CertificateFactory> getInstance(std::shared_ptr<FakeJni::JString> s);
    std::shared_ptr<Certificate> generateCertificate(std::shared_ptr<InputStream> s);
};

class TrustManager : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("javax/net/ssl/TrustManager")
};

class X509TrustManager : public TrustManager {
public:
    DEFINE_CLASS_NAME("javax/net/ssl/X509TrustManager", TrustManager)
};

class TrustManagerFactory : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("javax/net/ssl/TrustManagerFactory")
    static std::shared_ptr<TrustManagerFactory> getInstance(std::shared_ptr<FakeJni::JString> s);
    std::shared_ptr<FakeJni::JArray<TrustManager>> getTrustManagers();
};

class StrictHostnameVerifier : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("org/apache/http/conn/ssl/StrictHostnameVerifier")
    StrictHostnameVerifier();
    void verify(std::shared_ptr<FakeJni::JString> s, std::shared_ptr<X509Certificate> cert);
};
