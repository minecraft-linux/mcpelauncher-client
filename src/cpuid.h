#pragma once

class CpuId {
private:
    unsigned int hiLeaf;
    unsigned int hiExtLeaf;
    char manufacturerId[4 * 3 + 1];
    bool hasBrandString = false;
    char brandString[48 + 1];
    bool hasFeatureFlags = false;
    int featureFlagsC = 0, featureFlagsD = 0;

    static void cpuid(int data[4], int leaf);

    void queryFeatureFlags();

public:
    enum class FeatureFlag : unsigned char {
        SSSE3 = 9
    };

    CpuId();

    const char* getManufacturer() {
        return manufacturerId;
    }

    const char* getBrandString();

    bool queryFeatureFlag(FeatureFlag flag);
};
