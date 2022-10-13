#include "armhf_support.h"

#include <game_window_manager.h>

#define WRAP_DEF_ORIG(name, ...) static void (*name##_orig)(__VA_ARGS__);
#define WRAP_DEF_FUNC(name, ...) static void name##_wrap(__VA_ARGS__)
#define WRAP_DEF_FUNC_C(name, ...) name##_orig(__VA_ARGS__);

#define WRAP1(name, sa, da)  \
    WRAP_DEF_ORIG(name, da); \
    WRAP_DEF_FUNC(name, sa v1) { WRAP_DEF_FUNC_C(name, (da&)v1); }
#define WRAP2(name, sa, sb, da, db)              \
    WRAP_DEF_ORIG(name, da, db);                 \
    WRAP_DEF_FUNC(name, sa v1, sb v2) {          \
        WRAP_DEF_FUNC_C(name, (da&)v1, (db&)v2); \
    }
#define WRAP3(name, sa, sb, sc, da, db, dc)               \
    WRAP_DEF_ORIG(name, da, db, dc);                      \
    WRAP_DEF_FUNC(name, sa v1, sb v2, sc v3) {            \
        WRAP_DEF_FUNC_C(name, (da&)v1, (db&)v2, (dc&)v3); \
    }
#define WRAP4(name, sa, sb, sc, sd, da, db, dc, dd)                \
    WRAP_DEF_ORIG(name, da, db, dc, dd);                           \
    WRAP_DEF_FUNC(name, sa v1, sb v2, sc v3, sd v4) {              \
        WRAP_DEF_FUNC_C(name, (da&)v1, (db&)v2, (dc&)v3, (dd&)v4); \
    }

WRAP4(glClearColor, int, int, int, int, float, float, float, float)
WRAP2(glUniform1f, int, int, int, float)
WRAP1(glClearDepthf, int, float)
WRAP2(glDepthRangef, int, int, float, float)

#define WRAP_INSTALL_GL(name)                     \
    (void* (*&)()) name##_orig = procFunc(#name); \
    overrides[#name] = (void*)name##_wrap;

void ArmhfSupport::install(std::unordered_map<std::string, void*>& overrides) {
    auto procFunc = GameWindowManager::getManager()->getProcAddrFunc();
    WRAP_INSTALL_GL(glClearColor)
    WRAP_INSTALL_GL(glUniform1f)
    WRAP_INSTALL_GL(glClearDepthf)
    WRAP_INSTALL_GL(glDepthRangef)
}