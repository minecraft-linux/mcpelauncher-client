#include "armhf_support.h"

#include <game_window_manager.h>

#include "glad/glad.h"

template<auto F, class T> struct wrapOpenGLESImpl;
template<auto F, class R, class ...T> struct wrapOpenGLESImpl<F, R(**)(T...)> {
    static constexpr __attribute__((pcs("aapcs"))) R invoke(T... args) {
        return (*F)(args...);
    }
    static void* Get() {
        if constexpr(std::is_same_v<R, GLfloat> || std::is_same_v<R, GLdouble> || ((std::is_same_v<T, GLfloat>||std::is_same_v<T, GLdouble>)||...)) {
           return (void*)&invoke;
        }
        return (void*)*F;
    }
};

template<auto F> struct wrapOpenGLES : wrapOpenGLESImpl<F, decltype(F)> {
    
};

void ArmhfSupport::install(std::unordered_map<std::string, void*>& overrides) {
    auto procFunc = GameWindowManager::getManager()->getProcAddrFunc();
#include "opengl_es_2_map.h"
}
