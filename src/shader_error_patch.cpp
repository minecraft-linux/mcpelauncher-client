#include "shader_error_patch.h"
#include <mcpelauncher/linker.h>
#include <game_window_manager.h>
#include <log.h>

void (*ShaderErrorPatch::glGetShaderiv)(unsigned int shader, int pname, int* params);
void (*ShaderErrorPatch::glGetShaderInfoLog)(unsigned int shader, unsigned int maxLength, unsigned int* length, char* log);
void (*ShaderErrorPatch::glCompileShader)(unsigned int shader);
void (*ShaderErrorPatch::glGetProgramiv)(unsigned int program, int pname, int* params);
void (*ShaderErrorPatch::glGetProgramInfoLog)(unsigned int program, unsigned int maxLength, unsigned int* length, char* log);
void (*ShaderErrorPatch::glLinkProgram)(unsigned int program);

void ShaderErrorPatch::install(void* handle) {
    // TODO:
    //    hybris_hook("glCompileShader", (void*) glCompileShaderHook);
    //    hybris_hook("glLinkProgram", (void*) glLinkProgramHook);
}

void ShaderErrorPatch::onGLContextCreated() {
    auto getProcAddr = GameWindowManager::getManager()->getProcAddrFunc();
    glCompileShader = (void (*)(unsigned int))getProcAddr("glCompileShader");
    glLinkProgram = (void (*)(unsigned int))getProcAddr("glLinkProgram");
    glGetShaderiv = (void (*)(unsigned int, int, int*))getProcAddr("glGetShaderiv");
    glGetShaderInfoLog = (void (*)(unsigned int, unsigned int, unsigned int*, char*))getProcAddr("glGetShaderInfoLog");
    glGetProgramiv = (void (*)(unsigned int, int, int*))getProcAddr("glGetProgramiv");
    glGetProgramInfoLog = (void (*)(unsigned int, unsigned int, unsigned int*, char*))getProcAddr("glGetProgramInfoLog");
}

void ShaderErrorPatch::glCompileShaderHook(unsigned int shader) {
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, /* GL_COMPILE_STATUS */ 0x8B81, &status);
    if(status != /* GL_TRUE */ 1) {
        Log::error("Shader", "An error was detected when compiling a shader");
        unsigned int infoLen;
        glGetShaderiv(shader, /* GL_INFO_LOG_LENGTH */ 0x8B84, (int*)&infoLen);
        char* data = new char[infoLen];
        glGetShaderInfoLog(shader, infoLen, &infoLen, data);
        printf("%s\n", data);  // use printf because the logger may have a restricted length
        fflush(stdout);
    }
}

void ShaderErrorPatch::glLinkProgramHook(unsigned int program) {
    glLinkProgram(program);
    int status;
    glGetProgramiv(program, /* GL_LINK_STATUS */ 0x8B82, &status);
    if(status != /* GL_TRUE */ 1) {
        Log::error("Shader", "An error was detected when linking a shader program");
        unsigned int infoLen;
        glGetProgramiv(program, /* GL_INFO_LOG_LENGTH */ 0x8B84, (int*)&infoLen);
        char* data = new char[infoLen];
        glGetProgramInfoLog(program, infoLen, &infoLen, data);
        printf("%s\n", data);  // use printf because the logger may have a restricted length
        fflush(stdout);
    }
}
