/*
 * Protobuf conflict fix for mcpelauncher-ui-qt on macOS 26
 *
 * Two copies of Google protobuf get loaded: the app's bundled
 * libprotobuf.32.dylib and one compiled into Apple's MLAssetIO
 * (pulled in transitively by CoreML via Espresso). Their shared
 * global state (ShutdownData) gets corrupted, and OnShutdownRun
 * dereferences the sentinel 0xBAD4007 as a function pointer.
 *
 * Fix: DYLD_FORCE_FLAT_NAMESPACE=1 collapses all protobuf symbol
 * references to a single copy, eliminating the dual-state problem.
 * This tiny shim library is loaded first via DYLD_INSERT_LIBRARIES
 * to guard the one dangerous entry point in case flat namespace
 * alone isn't sufficient on future OS updates.
 */

#include <stdint.h>

typedef void (*pb_cb_arg)(const void*);
typedef void (*pb_cb_void)(void);

/* Direct symbol override -- loaded before libprotobuf.32.dylib
 * so these land in the flat-namespace symbol table first. They
 * forward to the "real" copy via dlsym(RTLD_NEXT, ...) only after
 * validating the callback pointer isn't a poison value.
 */

#include <dlfcn.h>

static int fn_ok(uintptr_t p) {
    return p > 0x10000 && p != 0xBAD4007 && p != 0xDEADBEEF;
}

void _ZN6google8protobuf8internal13OnShutdownRunEPFvPKvES3_(
        pb_cb_arg f, const void *arg) {
    if (!fn_ok((uintptr_t)f)) return;
    typedef void (*real_t)(pb_cb_arg, const void*);
    real_t real = (real_t)dlsym(RTLD_NEXT,
        "_ZN6google8protobuf8internal13OnShutdownRunEPFvPKvES3_");
    if (real && fn_ok((uintptr_t)real))
        real(f, arg);
}

void _ZN6google8protobuf8internal10OnShutdownEPFvvE(pb_cb_void f) {
    if (!fn_ok((uintptr_t)f)) return;
    typedef void (*real_t)(pb_cb_void);
    real_t real = (real_t)dlsym(RTLD_NEXT,
        "_ZN6google8protobuf8internal10OnShutdownEPFvvE");
    if (real && fn_ok((uintptr_t)real))
        real(f);
}
