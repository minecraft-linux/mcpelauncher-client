#include "main_activity.h"

#include <random>
#include <unistd.h>
#include <sys/sysinfo.h>

FakeJni::JInt BuildVersion::SDK_INT = 27;

std::shared_ptr<FakeJni::JString> MainActivity::createUUID() {
    static std::independent_bits_engine<std::random_device, CHAR_BIT, unsigned char> engine;
    unsigned char rawBytes[16];
    std::generate(rawBytes, rawBytes + 16, std::ref(engine));
    rawBytes[6] = (rawBytes[6] & (unsigned char) 0x0Fu) | (unsigned char) 0x40u;
    rawBytes[8] = (rawBytes[6] & (unsigned char) 0x3Fu) | (unsigned char) 0x80u;
    std::string ret;
    ret.resize(36);
    snprintf((char*) ret.c_str(), 36, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             rawBytes[0], rawBytes[1], rawBytes[2], rawBytes[3],
             rawBytes[4], rawBytes[5], rawBytes[6], rawBytes[7], rawBytes[8], rawBytes[9],
             rawBytes[10], rawBytes[11], rawBytes[12], rawBytes[13], rawBytes[14], rawBytes[15]);
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

FakeJni::JLong MainActivity::getMemoryLimit() {
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

FakeJni::JLong MainActivity::getAvailableMemory() {
    return getMemoryLimit();
}