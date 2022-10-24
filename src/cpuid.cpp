#include "cpuid.h"

#include <string.h>

void CpuId::cpuid(int* data, int leaf) {
    __asm__("cpuid"
            : "=a"(data[0]), "=b"(data[1]), "=c"(data[2]), "=d"(data[3])
            : "a"(leaf));
}

CpuId::CpuId() {
    int data[4];
    cpuid(data, 0);
    hiLeaf = (unsigned int)data[0];
    memcpy(&manufacturerId[0], &data[1], 4);
    memcpy(&manufacturerId[4], &data[3], 4);
    memcpy(&manufacturerId[8], &data[2], 4);
    manufacturerId[12] = 0;
    cpuid(data, 0x80000000);
    hiExtLeaf = (unsigned int)data[0];
}

const char* CpuId::getBrandString() {
    if(hasBrandString)
        return brandString;
    if(hiExtLeaf < 0x80000004)
        return nullptr;
    int data[4];
    for(int i = 0; i < 3; i++) {
        cpuid(data, 0x80000002 + i);
        memcpy(&brandString[i * 16], data, 16);
    }
    brandString[48] = 0;
    return brandString;
}

void CpuId::queryFeatureFlags() {
    if(hasFeatureFlags)
        return;
    if(hiLeaf < 1)
        return;
    hasFeatureFlags = true;
    int data[4];
    cpuid(data, 1);
    featureFlagsC = data[2];
    featureFlagsD = data[3];
}

bool CpuId::queryFeatureFlag(CpuId::FeatureFlag flag) {
    queryFeatureFlags();
    auto flagi = (unsigned char)flag;
    if(flagi & 128)
        return (featureFlagsD & (1 << (flagi & 0x7f))) != 0;
    else
        return (featureFlagsC & (1 << (flagi & 0x7f))) != 0;
}
