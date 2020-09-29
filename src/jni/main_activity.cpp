#include "main_activity.h"

#include <random>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/sysinfo.h>
#else
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif
#include <file_picker_factory.h>
#include <climits>
#include <sstream>

FakeJni::JInt BuildVersion::SDK_INT = 27;
std::shared_ptr<FakeJni::JString> BuildVersion::RELEASE = std::make_shared<FakeJni::JString>("AndroidX");

std::shared_ptr<FakeJni::JString> MainActivity::createUUID() {
    static std::independent_bits_engine<std::random_device, CHAR_BIT, unsigned char> engine;
    unsigned char rawBytes[16];
    std::generate(rawBytes, rawBytes + 16, std::ref(engine));
    rawBytes[6] = (rawBytes[6] & (unsigned char) 0x0Fu) | (unsigned char) 0x40u;
    rawBytes[8] = (rawBytes[6] & (unsigned char) 0x3Fu) | (unsigned char) 0x80u;
    std::string ret;
    ret.resize(37);
    snprintf((char*) ret.c_str(), 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             rawBytes[0], rawBytes[1], rawBytes[2], rawBytes[3],
             rawBytes[4], rawBytes[5], rawBytes[6], rawBytes[7], rawBytes[8], rawBytes[9],
             rawBytes[10], rawBytes[11], rawBytes[12], rawBytes[13], rawBytes[14], rawBytes[15]);
    ret.resize(36);
    return std::make_shared<FakeJni::JString>(ret);
}

FakeJni::JLong MainActivity::getUsedMemory() {
#ifdef __APPLE__
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    struct vm_statistics64 stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) &stat, &count);

    double page_K = page_size / (double) 1024;
    return (stat.active_count + stat.wire_count) * page_K * 1000;
#else
    FILE* file = fopen("/proc/self/statm", "r");
    if (file == nullptr)
        return 0L;
    int pageSize = getpagesize();
    long long pageCount = 0L;
    fscanf(file, "%lld", &pageCount);
    fclose(file);
    return pageCount * pageSize;
#endif
}

FakeJni::JLong MainActivity::getFreeMemory() {
#ifdef __APPLE__
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    struct vm_statistics64 stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) &stat, &count);

    double page_K = page_size / (double) 1024;
    return stat.free_count * page_K * 1000;
#else
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    long long total = memInfo.freeram;
    total *= memInfo.mem_unit;
    return total;
#endif
}

FakeJni::JLong MainActivity::getTotalMemory() {
#ifdef __APPLE__
    uint64_t memsize;
    size_t len = sizeof(memsize);
    sysctlbyname("hw.memsize", &memsize, &len, NULL, 0);
    return memsize;
#else
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    long long total = memInfo.totalram;
    total *= memInfo.mem_unit;
    return total;
#endif
}

FakeJni::JLong MainActivity::getMemoryLimit() {
    return getTotalMemory();
}

FakeJni::JLong MainActivity::getAvailableMemory() {
    return getTotalMemory();
}
#include <thread>
void MainActivity::pickImage(FakeJni::JLong callback) {
    auto&& vm = FakeJni::JniEnvContext().getJniEnv().getVM();
    std::thread([vm=&vm, callback, this]() {
        auto that = shared_from_this();
        auto picker = FilePickerFactory::createFilePicker();
        picker->setTitle("Select image");
        picker->setFileNameFilters({ "*.png" });
        if (picker->show()) {
            auto method = MainActivity::getDescriptor()->getMethod("(JLjava/lang/String;)V", "nativeOnPickImageSuccess");
            FakeJni::LocalFrame frame(*vm);
            method->invoke(frame.getJniEnv(), this, callback, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(picker->getPickedFile())));
        } else {
            auto method = MainActivity::getDescriptor()->getMethod("(J)V", "nativeOnPickImageCanceled");
            FakeJni::LocalFrame frame(*vm);
            method->invoke(frame.getJniEnv(), this, callback);
        }
    }).detach();
}

void MainActivity::initializeXboxLive(FakeJni::JLong xalinit, FakeJni::JLong xblinit) {
        auto method = getClass().getMethod("(JJ)V", "nativeInitializeXboxLive");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, xalinit, xblinit);
}

std::shared_ptr<FakeJni::JIntArray> MainActivity::getImageData(std::shared_ptr<FakeJni::JString> filename, jboolean b) {
    if(!stbi_load_from_memory || !stbi_image_free) return 0;
    int width, height, channels;
    std::ifstream f(filename->asStdString().data());
    if(!f.is_open()) return 0;
    std::stringstream s;
    s << f.rdbuf();
    auto buf = s.str();
    auto image = stbi_load_from_memory((unsigned char*)buf.data(), buf.length(), &width, &height, &channels, 4);
    if(!image) return 0;
    auto ret = std::make_shared<FakeJni::JIntArray>(2 + width * height);
    (*ret)[0] = width;
    (*ret)[1] = height;

    for(int x = 0; x < width * height; x++) {
        (*ret)[2 + x] = (image[x * 4 + 2]) | (image[x * 4 + 1] << 8) | (image[x * 4 + 0] << 16) | (image[x * 4 + 3] << 24);
    }
    stbi_image_free(image);
    return ret;
} 