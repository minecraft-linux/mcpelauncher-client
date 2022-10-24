#pragma once

#include <cstddef>

class ShaderErrorPatch {
private:
    static void (*glGetShaderiv)(unsigned int shader, int pname, int* params);
    static void (*glGetShaderInfoLog)(unsigned int shader, unsigned int maxLength, unsigned int* length, char* log);

    static void (*glCompileShader)(unsigned int shader);
    static void glCompileShaderHook(unsigned int shader);

    static void (*glGetProgramiv)(unsigned int program, int pname, int* params);
    static void (*glGetProgramInfoLog)(unsigned int program, unsigned int maxLength, unsigned int* length, char* log);

    static void (*glLinkProgram)(unsigned int program);
    static void glLinkProgramHook(unsigned int program);

public:
    static void install(void* handle);

    static void onGLContextCreated();
};
