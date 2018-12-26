#include "shader_error_patch.h"
#include <hybris/dlfcn.h>
#include <game_window_manager.h>
#include <hybris/hook.h>
#include <log.h>

void (*ShaderErrorPatch::glGetShaderiv)(unsigned int shader, int pname, int* params);
void (*ShaderErrorPatch::glGetShaderInfoLog)(unsigned int shader, unsigned int maxLength, unsigned int* length, char* log);
void (*ShaderErrorPatch::glCompileShader)(unsigned int shader);

void ShaderErrorPatch::install(void *handle) {
    hybris_hook("glCompileShader", (void*) glCompileShaderHook);
}

void ShaderErrorPatch::onGLContextCreated() {
    auto getProcAddr = GameWindowManager::getManager()->getProcAddrFunc();
    glCompileShader = (void (*)(unsigned int)) getProcAddr("glCompileShader");
    glGetShaderiv = (void (*)(unsigned int, int, int*)) getProcAddr("glGetShaderiv");
    glGetShaderInfoLog = (void (*)(unsigned int, unsigned int, unsigned int*, char*)) getProcAddr("glGetShaderInfoLog");
}

void ShaderErrorPatch::glCompileShaderHook(unsigned int shader) {
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, /* GL_COMPILE_STATUS */ 0x8B81, &status);
    if (status != /* GL_TRUE */ 1) {
        Log::error("Shader", "An error was detected when compiling a shader");
        unsigned int infoLen;
        glGetShaderiv(shader, /* GL_INFO_LOG_LENGTH */ 0x8B84, (int*) &infoLen);
        char* data = new char[infoLen];
        glGetShaderInfoLog(shader, infoLen, &infoLen, data);
        printf("%s\n", data); // use printf because the logger may have a restricted length
        fflush(stdout);
    }
}