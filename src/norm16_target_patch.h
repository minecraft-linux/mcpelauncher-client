#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Norm16TargetPatch {
private:
    struct TextureSize {
        int width;
        int height;
    };

    static std::unordered_map<unsigned int, TextureSize> integerTargets;
    static std::unordered_map<unsigned int, std::vector<int>> floatOutputLocationsByProgram;
    static std::unordered_set<uint64_t> checkedPairs;

    static unsigned int currentProgram;
    static unsigned int currentDrawFramebuffer;
    static bool pairChanged;

    static void (*glTexImage2D_orig)(unsigned int target, int level, int internalFormat, int width, int height, int border, unsigned int format, unsigned int type, const void *data);
    static void (*glTexStorage2D_orig)(unsigned int target, int levels, unsigned int internalFormat, int width, int height);
    static void (*glUseProgram_orig)(unsigned int program);
    static void (*glLinkProgram_orig)(unsigned int program);
    static void (*glBindFramebuffer_orig)(unsigned int target, unsigned int framebuffer);
    static void (*glFramebufferTexture2D_orig)(unsigned int target, unsigned int attachment, unsigned int textureTarget, unsigned int texture, int level);
    static void (*glDrawArrays_orig)(unsigned int mode, int first, int count);
    static void (*glDrawElements_orig)(unsigned int mode, int count, unsigned int type, const void *indices);
    static void (*glDrawArraysInstanced_orig)(unsigned int mode, int first, int count, int instanceCount);
    static void (*glDrawElementsInstanced_orig)(unsigned int mode, int count, unsigned int type, const void *indices, int instanceCount);

    static void (*glGetIntegerv)(unsigned int name, int *value);
    static void (*glBindTexture)(unsigned int target, unsigned int texture);
    static void (*glGetProgramInterfaceiv)(unsigned int program, unsigned int programInterface, unsigned int name, int *params);
    static void (*glGetProgramResourceiv)(unsigned int program, unsigned int programInterface, unsigned int index, int propCount, const unsigned int *props, int bufSize, int *length, int *params);
    static void (*glGetFramebufferAttachmentParameteriv)(unsigned int target, unsigned int attachment, unsigned int name, int *params);

    static unsigned int boundTexture2D();
    static unsigned int attachedTexture(int location);
    static const std::vector<int> &floatOutputLocations(unsigned int program);
    static void respecifyAsNorm16(unsigned int texture, TextureSize size);
    static void convertMismatchedTargets();

    static void glTexImage2D(unsigned int target, int level, int internalFormat, int width, int height, int border, unsigned int format, unsigned int type, const void *data);
    static void glTexStorage2D(unsigned int target, int levels, unsigned int internalFormat, int width, int height);
    static void glUseProgram(unsigned int program);
    static void glLinkProgram(unsigned int program);
    static void glBindFramebuffer(unsigned int target, unsigned int framebuffer);
    static void glFramebufferTexture2D(unsigned int target, unsigned int attachment, unsigned int textureTarget, unsigned int texture, int level);
    static void glDrawArrays(unsigned int mode, int first, int count);
    static void glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices);
    static void glDrawArraysInstanced(unsigned int mode, int first, int count, int instanceCount);
    static void glDrawElementsInstanced(unsigned int mode, int count, unsigned int type, const void *indices, int instanceCount);

public:
    static void install(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *));
};
