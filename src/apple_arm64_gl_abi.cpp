#include "apple_arm64_gl_abi.h"

#if defined(__APPLE__) && defined(__aarch64__)

#include <cstdint>

namespace {

// Declaring a stack argument as 64 bit makes the Apple callee read the same 8 byte slot the Android caller wrote.
using StackSlot = uint64_t;

template <int Variant>
struct BlitFramebuffer {
    static inline void (*target)(int, int, int, int, int, int, int, int, unsigned int, unsigned int);

    static void fromAndroid(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, StackSlot mask, StackSlot filter) {
        target(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, (unsigned int)mask, (unsigned int)filter);
    }
};

template <int Variant>
struct TexSubImage3D {
    static inline void (*target)(unsigned int, int, int, int, int, int, int, int, unsigned int, unsigned int, const void *);

    static void fromAndroid(unsigned int textureTarget, int level, int xOffset, int yOffset, int zOffset, int width, int height, int depth, StackSlot format, StackSlot type, const void *pixels) {
        target(textureTarget, level, xOffset, yOffset, zOffset, width, height, depth, (unsigned int)format, (unsigned int)type, pixels);
    }
};

template <int Variant>
struct CompressedTexSubImage3D {
    static inline void (*target)(unsigned int, int, int, int, int, int, int, int, unsigned int, int, const void *);

    static void fromAndroid(unsigned int textureTarget, int level, int xOffset, int yOffset, int zOffset, int width, int height, int depth, StackSlot format, StackSlot imageSize, const void *data) {
        target(textureTarget, level, xOffset, yOffset, zOffset, width, height, depth, (unsigned int)format, (int)imageSize, data);
    }
};

template <int Variant>
struct CopyImageSubData {
    static inline void (*target)(unsigned int, unsigned int, int, int, int, int, unsigned int, unsigned int, int, int, int, int, int, int, int);

    static void fromAndroid(unsigned int srcName, unsigned int srcTarget, int srcLevel, int srcX, int srcY, int srcZ, unsigned int dstName, unsigned int dstTarget, StackSlot dstLevel, StackSlot dstX, StackSlot dstY, StackSlot dstZ, StackSlot width, StackSlot height, StackSlot depth) {
        target(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, (int)dstLevel, (int)dstX, (int)dstY, (int)dstZ, (int)width, (int)height, (int)depth);
    }
};

template <class Wrapper>
void wrap(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *), const char *name) {
    auto existingOverride = overrides.find(name);
    void *target = existingOverride != overrides.end() ? existingOverride->second : resolver(name);

    if(!target) {
        return;
    }

    Wrapper::target = (decltype(Wrapper::target))target;
    overrides[name] = (void *)Wrapper::fromAndroid;
}

}  // namespace

void AppleArm64GLABI::install(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *)) {
    wrap<BlitFramebuffer<0>>(overrides, resolver, "glBlitFramebuffer");
    wrap<BlitFramebuffer<1>>(overrides, resolver, "glBlitFramebufferANGLE");
    wrap<BlitFramebuffer<2>>(overrides, resolver, "glBlitFramebufferNV");

    wrap<TexSubImage3D<0>>(overrides, resolver, "glTexSubImage3D");
    wrap<TexSubImage3D<1>>(overrides, resolver, "glTexSubImage3DOES");

    wrap<CompressedTexSubImage3D<0>>(overrides, resolver, "glCompressedTexSubImage3D");
    wrap<CompressedTexSubImage3D<1>>(overrides, resolver, "glCompressedTexSubImage3DOES");

    wrap<CopyImageSubData<0>>(overrides, resolver, "glCopyImageSubData");
    wrap<CopyImageSubData<1>>(overrides, resolver, "glCopyImageSubDataEXT");
    wrap<CopyImageSubData<2>>(overrides, resolver, "glCopyImageSubDataOES");
}

#else

void AppleArm64GLABI::install(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *)) {
}

#endif
