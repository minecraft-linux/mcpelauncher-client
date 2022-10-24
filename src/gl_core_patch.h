#pragma once

#include <cstddef>
#include <unordered_map>
#include <string>

class GLCorePatch {
private:
    static bool enabled;
    static std::unordered_map<unsigned int, unsigned int> vaoMap;
    static std::pair<int, unsigned int> buffers[2];

    static void (*glGenVertexArrays)(int n, unsigned int *arrays);
    static void (*glBindVertexArray)(unsigned int array);

    static void (*glShaderSource_orig)(unsigned int shader, unsigned int count, const char **string, int *length);
    static void glShaderSource(unsigned int shader, unsigned int count, const char **string, int *length);

    static void (*glLinkProgram_orig)(unsigned int program);
    static void glLinkProgram(unsigned int program);

    static void (*glUseProgram_orig)(unsigned int program);
    static void glUseProgram(unsigned int program);

    static void (*glBindBuffer_orig)(int target, unsigned int buffer);
    static void glBindBuffer(int target, unsigned int buffer);

public:
    static void install(void *handle);

    static void installGL(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *));

    static bool mustUseDesktopGL();
};
