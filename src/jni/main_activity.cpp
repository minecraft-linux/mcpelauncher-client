#include "main_activity.h"

#include <unistd.h>
#ifndef __APPLE__
#include <sys/sysinfo.h>
#else
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif
#include <file_picker_factory.h>
#include "uuid.h"

FakeJni::JInt BuildVersion::SDK_INT = 27;
std::shared_ptr<FakeJni::JString> BuildVersion::RELEASE = std::make_shared<FakeJni::JString>("AndroidX");

std::shared_ptr<FakeJni::JString> MainActivity::createUUID() {
    return UUID::randomUUID()->toString();
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

void MainActivity::pickImage(FakeJni::JLong callback) {
    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle("Select image");
    picker->setFileNameFilters({ "*.png" });
    if (picker->show()) {
        auto method = getClass().getMethod("(JLjava/lang/String;)V", "nativeOnPickImageSuccess");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, callback, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(picker->getPickedFile())));
    } else {
        auto method = getClass().getMethod("(J)V", "nativeOnPickImageCanceled");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, callback);
    }
}

void MainActivity::initializeXboxLive(FakeJni::JLong xalinit, FakeJni::JLong xblinit) {
    auto method = getClass().getMethod("(JJ)V", "nativeInitializeXboxLive");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, xalinit, xblinit);
}
