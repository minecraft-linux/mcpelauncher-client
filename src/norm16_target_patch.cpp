#include "norm16_target_patch.h"
#include <log.h>

// The game creates its RGBA16 render targets as RGBA16UI on OpenGL ES, yet writes and samples some of them as floats.
// Metal rejects that, so such a target is recreated as normalized RGBA16 the first time a float shader draws into it.

namespace {

const unsigned int TEXTURE_2D = 0x0DE1;
const unsigned int TEXTURE_BINDING_2D = 0x8069;
const unsigned int TEXTURE = 0x1702;

const unsigned int RGBA = 0x1908;
const unsigned int RGBA16 = 0x805B;
const unsigned int RGBA16UI = 0x8D76;
const unsigned int UNSIGNED_SHORT = 0x1403;

const unsigned int FRAMEBUFFER = 0x8D40;
const unsigned int DRAW_FRAMEBUFFER = 0x8CA9;
const unsigned int COLOR_ATTACHMENT0 = 0x8CE0;
const unsigned int FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0;
const unsigned int FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1;
const int MAX_COLOR_ATTACHMENTS = 8;

const unsigned int PROGRAM_OUTPUT = 0x92E4;
const unsigned int ACTIVE_RESOURCES = 0x92F5;
const unsigned int TYPE = 0x92FA;
const unsigned int ARRAY_SIZE = 0x92FB;
const unsigned int LOCATION = 0x930E;

const int FLOAT = 0x1406;
const int FLOAT_VEC2 = 0x8B50;
const int FLOAT_VEC3 = 0x8B51;
const int FLOAT_VEC4 = 0x8B52;

bool isFloatType(int type) {
    return type == FLOAT || type == FLOAT_VEC2 || type == FLOAT_VEC3 || type == FLOAT_VEC4;
}

}  // namespace

std::unordered_map<unsigned int, Norm16TargetPatch::TextureSize> Norm16TargetPatch::integerTargets;
std::unordered_map<unsigned int, std::vector<int>> Norm16TargetPatch::floatOutputLocationsByProgram;
std::unordered_set<uint64_t> Norm16TargetPatch::checkedPairs;

unsigned int Norm16TargetPatch::currentProgram = 0;
unsigned int Norm16TargetPatch::currentDrawFramebuffer = 0;
bool Norm16TargetPatch::pairChanged = false;

void (*Norm16TargetPatch::glTexImage2D_orig)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *);
void (*Norm16TargetPatch::glTexStorage2D_orig)(unsigned int, int, unsigned int, int, int);
void (*Norm16TargetPatch::glUseProgram_orig)(unsigned int);
void (*Norm16TargetPatch::glLinkProgram_orig)(unsigned int);
void (*Norm16TargetPatch::glBindFramebuffer_orig)(unsigned int, unsigned int);
void (*Norm16TargetPatch::glFramebufferTexture2D_orig)(unsigned int, unsigned int, unsigned int, unsigned int, int);
void (*Norm16TargetPatch::glDrawArrays_orig)(unsigned int, int, int);
void (*Norm16TargetPatch::glDrawElements_orig)(unsigned int, int, unsigned int, const void *);
void (*Norm16TargetPatch::glDrawArraysInstanced_orig)(unsigned int, int, int, int);
void (*Norm16TargetPatch::glDrawElementsInstanced_orig)(unsigned int, int, unsigned int, const void *, int);

void (*Norm16TargetPatch::glGetIntegerv)(unsigned int, int *);
void (*Norm16TargetPatch::glBindTexture)(unsigned int, unsigned int);
void (*Norm16TargetPatch::glGetProgramInterfaceiv)(unsigned int, unsigned int, unsigned int, int *);
void (*Norm16TargetPatch::glGetProgramResourceiv)(unsigned int, unsigned int, unsigned int, int, const unsigned int *, int, int *, int *);
void (*Norm16TargetPatch::glGetFramebufferAttachmentParameteriv)(unsigned int, unsigned int, unsigned int, int *);

void Norm16TargetPatch::install(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *)) {
    glGetIntegerv = (void (*)(unsigned int, int *))resolver("glGetIntegerv");
    glBindTexture = (void (*)(unsigned int, unsigned int))resolver("glBindTexture");
    glGetProgramInterfaceiv = (void (*)(unsigned int, unsigned int, unsigned int, int *))resolver("glGetProgramInterfaceiv");
    glGetProgramResourceiv = (void (*)(unsigned int, unsigned int, unsigned int, int, const unsigned int *, int, int *, int *))resolver("glGetProgramResourceiv");
    glGetFramebufferAttachmentParameteriv = (void (*)(unsigned int, unsigned int, unsigned int, int *))resolver("glGetFramebufferAttachmentParameteriv");

    bool canInspectPrograms = glGetProgramInterfaceiv && glGetProgramResourceiv;

    if(!canInspectPrograms) {
        Log::warn("Norm16TargetPatch", "Program interface queries are unavailable, patch disabled");
        return;
    }

    glTexImage2D_orig = (void (*)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *))resolver("glTexImage2D");
    glTexStorage2D_orig = (void (*)(unsigned int, int, unsigned int, int, int))resolver("glTexStorage2D");
    glUseProgram_orig = (void (*)(unsigned int))resolver("glUseProgram");
    glLinkProgram_orig = (void (*)(unsigned int))resolver("glLinkProgram");
    glBindFramebuffer_orig = (void (*)(unsigned int, unsigned int))resolver("glBindFramebuffer");
    glFramebufferTexture2D_orig = (void (*)(unsigned int, unsigned int, unsigned int, unsigned int, int))resolver("glFramebufferTexture2D");
    glDrawArrays_orig = (void (*)(unsigned int, int, int))resolver("glDrawArrays");
    glDrawElements_orig = (void (*)(unsigned int, int, unsigned int, const void *))resolver("glDrawElements");
    glDrawArraysInstanced_orig = (void (*)(unsigned int, int, int, int))resolver("glDrawArraysInstanced");
    glDrawElementsInstanced_orig = (void (*)(unsigned int, int, unsigned int, const void *, int))resolver("glDrawElementsInstanced");

    overrides["glTexImage2D"] = (void *)glTexImage2D;
    overrides["glTexStorage2D"] = (void *)glTexStorage2D;
    overrides["glUseProgram"] = (void *)glUseProgram;
    overrides["glLinkProgram"] = (void *)glLinkProgram;
    overrides["glBindFramebuffer"] = (void *)glBindFramebuffer;
    overrides["glFramebufferTexture2D"] = (void *)glFramebufferTexture2D;
    overrides["glDrawArrays"] = (void *)glDrawArrays;
    overrides["glDrawElements"] = (void *)glDrawElements;
    overrides["glDrawArraysInstanced"] = (void *)glDrawArraysInstanced;
    overrides["glDrawElementsInstanced"] = (void *)glDrawElementsInstanced;
}

unsigned int Norm16TargetPatch::boundTexture2D() {
    int texture = 0;
    glGetIntegerv(TEXTURE_BINDING_2D, &texture);

    return (unsigned int)texture;
}

unsigned int Norm16TargetPatch::attachedTexture(int location) {
    unsigned int attachment = COLOR_ATTACHMENT0 + location;

    int objectType = 0;
    glGetFramebufferAttachmentParameteriv(DRAW_FRAMEBUFFER, attachment, FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);

    if((unsigned int)objectType != TEXTURE) {
        return 0;
    }

    int texture = 0;
    glGetFramebufferAttachmentParameteriv(DRAW_FRAMEBUFFER, attachment, FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &texture);

    return (unsigned int)texture;
}

const std::vector<int> &Norm16TargetPatch::floatOutputLocations(unsigned int program) {
    auto cached = floatOutputLocationsByProgram.find(program);

    if(cached != floatOutputLocationsByProgram.end()) {
        return cached->second;
    }

    std::vector<int> &locations = floatOutputLocationsByProgram[program];

    int outputCount = 0;
    glGetProgramInterfaceiv(program, PROGRAM_OUTPUT, ACTIVE_RESOURCES, &outputCount);

    const unsigned int props[] = {TYPE, LOCATION, ARRAY_SIZE};

    for(int index = 0; index < outputCount; index++) {
        int values[3] = {0, -1, 1};
        glGetProgramResourceiv(program, PROGRAM_OUTPUT, index, 3, props, 3, nullptr, values);

        int type = values[0];
        int firstLocation = values[1];
        int arraySize = values[2];

        if(!isFloatType(type) || firstLocation < 0) {
            continue;
        }

        for(int offset = 0; offset < arraySize; offset++) {
            int location = firstLocation + offset;

            if(location < MAX_COLOR_ATTACHMENTS) {
                locations.push_back(location);
            }
        }
    }

    return locations;
}

void Norm16TargetPatch::respecifyAsNorm16(unsigned int texture, TextureSize size) {
    unsigned int previousTexture = boundTexture2D();

    glBindTexture(TEXTURE_2D, texture);
    glTexImage2D_orig(TEXTURE_2D, 0, RGBA16, size.width, size.height, 0, RGBA, UNSIGNED_SHORT, nullptr);
    glBindTexture(TEXTURE_2D, previousTexture);

    Log::info("Norm16TargetPatch", "Recreated texture %u (%dx%d) as normalized RGBA16", texture, size.width, size.height);
}

void Norm16TargetPatch::convertMismatchedTargets() {
    if(!pairChanged) {
        return;
    }

    pairChanged = false;

    if(integerTargets.empty() || currentDrawFramebuffer == 0 || currentProgram == 0) {
        return;
    }

    uint64_t pair = ((uint64_t)currentProgram << 32) | currentDrawFramebuffer;
    bool alreadyChecked = !checkedPairs.insert(pair).second;

    if(alreadyChecked) {
        return;
    }

    for(int location : floatOutputLocations(currentProgram)) {
        unsigned int texture = attachedTexture(location);
        auto target = integerTargets.find(texture);

        if(target == integerTargets.end()) {
            continue;
        }

        respecifyAsNorm16(texture, target->second);
        integerTargets.erase(target);
    }
}

void Norm16TargetPatch::glTexImage2D(unsigned int target, int level, int internalFormat, int width, int height, int border, unsigned int format, unsigned int type, const void *data) {
    glTexImage2D_orig(target, level, internalFormat, width, height, border, format, type, data);

    if(target != TEXTURE_2D || level != 0) {
        return;
    }

    unsigned int texture = boundTexture2D();

    if((unsigned int)internalFormat != RGBA16UI) {
        integerTargets.erase(texture);
        return;
    }

    integerTargets[texture] = {width, height};
    checkedPairs.clear();
    pairChanged = true;
}

void Norm16TargetPatch::glTexStorage2D(unsigned int target, int levels, unsigned int internalFormat, int width, int height) {
    glTexStorage2D_orig(target, levels, internalFormat, width, height);

    if(target == TEXTURE_2D) {
        integerTargets.erase(boundTexture2D());
    }
}

void Norm16TargetPatch::glUseProgram(unsigned int program) {
    glUseProgram_orig(program);

    currentProgram = program;
    pairChanged = true;
}

void Norm16TargetPatch::glLinkProgram(unsigned int program) {
    glLinkProgram_orig(program);

    floatOutputLocationsByProgram.erase(program);
    checkedPairs.clear();
    pairChanged = true;
}

void Norm16TargetPatch::glBindFramebuffer(unsigned int target, unsigned int framebuffer) {
    glBindFramebuffer_orig(target, framebuffer);

    if(target == FRAMEBUFFER || target == DRAW_FRAMEBUFFER) {
        currentDrawFramebuffer = framebuffer;
        pairChanged = true;
    }
}

void Norm16TargetPatch::glFramebufferTexture2D(unsigned int target, unsigned int attachment, unsigned int textureTarget, unsigned int texture, int level) {
    glFramebufferTexture2D_orig(target, attachment, textureTarget, texture, level);

    checkedPairs.clear();
    pairChanged = true;
}

void Norm16TargetPatch::glDrawArrays(unsigned int mode, int first, int count) {
    convertMismatchedTargets();
    glDrawArrays_orig(mode, first, count);
}

void Norm16TargetPatch::glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices) {
    convertMismatchedTargets();
    glDrawElements_orig(mode, count, type, indices);
}

void Norm16TargetPatch::glDrawArraysInstanced(unsigned int mode, int first, int count, int instanceCount) {
    convertMismatchedTargets();
    glDrawArraysInstanced_orig(mode, first, count, instanceCount);
}

void Norm16TargetPatch::glDrawElementsInstanced(unsigned int mode, int count, unsigned int type, const void *indices, int instanceCount) {
    convertMismatchedTargets();
    glDrawElementsInstanced_orig(mode, count, type, indices, instanceCount);
}
