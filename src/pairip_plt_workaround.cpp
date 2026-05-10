// Workaround for Google PairIP DRM protection of libPlayFabMultiplayer.so on
// Android x86_64 builds.
//
// Background: PairIP (Google Play's binary protection) deliberately scrambles
// the PLT and .got.plt of libPlayFabMultiplayer.so, leaving most slots without
// proper R_X86_64_JUMP_SLOT relocations. It also encrypts the .data section,
// turning emutls_control structs into garbage. On Android, PairIP's DT_INIT
// fixes these up at runtime; on Linux desktop, that init code does not run,
// so the library crashes shortly after the game's main thread starts.
//
// This file mirrors the workaround that previously lived inside our patched
// bionic linker (mcpelauncher-linker/bionic/linker/linker_relocate.cpp). Per
// upstream-bionic discussion, DRM workarounds do not belong in the linker;
// they belong here, alongside the existing PairIP-related fixups
// (e.g. the libc++_shared.so __emutls_get_address sanitizing hook).
//
// Scope:
//   - Walk libPlayFabMultiplayer.so's .got.plt and patch known unrelocated
//     slots with glibc / bionic-compatible implementations.
//   - Set up a fallback resolver trampoline + GOT[2] for unknown slots.
//   - Sanitize known-bad emutls_control structs in libPlayFab .data.
//   - Hook libPlayFab's __emutls_get_address GOT slot with a sanitizer.
//   - Patch libPlayFab's __emutls_get_address function entry with an inline
//     trampoline so direct callers (libminecraftpe.so via global symbol
//     interposition) are also caught.
//
// Architecture: x86_64 only. On other architectures the entry point is a
// no-op stub, so callers do not need an #ifdef.

#include "pairip_plt_workaround.h"

#if defined(__x86_64__)

#define _GNU_SOURCE 1

#include <log.h>
#include <mcpelauncher/linker.h>

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <locale.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include <atomic>
#include <unordered_set>

// Futex constants (mirroring linux/futex.h, kept local to avoid pulling in the
// kernel header on platforms that miss it).
#ifndef FUTEX_WAIT
#define FUTEX_WAIT 0
#endif
#ifndef FUTEX_WAKE
#define FUTEX_WAKE 1
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

namespace {

// ---------------------------------------------------------------------------
// No-op stub for unresolved PLT entries.
// Returns 0 in rax, which is "success" for many int-returning functions
// (e.g. pthread_mutex_lock returns 0 on success).
// ---------------------------------------------------------------------------
int unrelocated_plt_noop(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// Bionic-compatible pthread_cond_* shims.
//
// libPlayFabMultiplayer.so was built statically against bionic and uses
// bionic's native futex-based pthread_cond_t layout (LP64):
//
//   atomic_uint state;       // offset 0, 4 bytes
//   char __reserved[44];     // offset 4
//
// state field bits:
//   bit 0:   COND_SHARED_MASK  (process-shared flag)
//   bit 1:   COND_CLOCK_MASK   (clock type)
//   bits 2+: counter           (incremented by signal/broadcast)
//
// glibc's pthread_cond_t layout differs entirely, so we cannot simply forward
// to glibc here.
//
// Wait protocol: load state, unlock mutex, futex_wait on state, relock mutex.
// Signal/broadcast: increment counter by 4 (skipping flag bits), futex_wake.
//
// Note: glibc's pthread_mutex_lock/unlock works with bionic-format NORMAL
// mutexes on x86_64 LE because both encodings use 0/1/2 (unlocked/locked/
// contended) in the first 4 bytes at offset 0.
// ---------------------------------------------------------------------------
int bionic_pthread_cond_timedwait(void* cond, void* mutex, const struct timespec* abstime) {
    unsigned int* state_ptr = reinterpret_cast<unsigned int*>(cond);
    unsigned int old_state = __atomic_load_n(state_ptr, __ATOMIC_RELAXED);
    int futex_op = FUTEX_WAIT;
    if (!(old_state & 1))
        futex_op |= FUTEX_PRIVATE_FLAG;

    // Convert absolute time to relative timeout for futex.
    struct timespec now, rel;
    clock_gettime(CLOCK_REALTIME, &now);
    rel.tv_sec = abstime->tv_sec - now.tv_sec;
    rel.tv_nsec = abstime->tv_nsec - now.tv_nsec;
    if (rel.tv_nsec < 0) {
        rel.tv_sec--;
        rel.tv_nsec += 1000000000L;
    }
    if (rel.tv_sec < 0) {
        return 110; // ETIMEDOUT on Android/bionic
    }

    pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mutex));
    long ret = syscall(SYS_futex, state_ptr, futex_op, old_state, &rel, nullptr, 0);
    int saved_errno = errno;
    pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mutex));
    if (ret == -1 && saved_errno == ETIMEDOUT)
        return 110;
    return 0;
}

int bionic_pthread_cond_wait(void* cond, void* mutex) {
    unsigned int* state_ptr = reinterpret_cast<unsigned int*>(cond);
    unsigned int old_state = __atomic_load_n(state_ptr, __ATOMIC_RELAXED);

    int futex_op = FUTEX_WAIT;
    if (!(old_state & 1))
        futex_op |= FUTEX_PRIVATE_FLAG;

    pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mutex));
    syscall(SYS_futex, state_ptr, futex_op, old_state, nullptr, nullptr, 0);
    pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mutex));

    return 0;
}

int bionic_pthread_cond_broadcast(void* cond) {
    unsigned int* state_ptr = reinterpret_cast<unsigned int*>(cond);
    unsigned int old_state = __atomic_load_n(state_ptr, __ATOMIC_RELAXED);

    int futex_op = FUTEX_WAKE;
    if (!(old_state & 1))
        futex_op |= FUTEX_PRIVATE_FLAG;

    // Increment counter by 4 (COND_COUNTER_STEP), skipping the 2 flag bits.
    __atomic_fetch_add(state_ptr, 4, __ATOMIC_RELAXED);

    syscall(SYS_futex, state_ptr, futex_op, INT_MAX, nullptr, nullptr, 0);
    return 0;
}

int bionic_pthread_cond_signal(void* cond) {
    unsigned int* state_ptr = reinterpret_cast<unsigned int*>(cond);
    unsigned int old_state = __atomic_load_n(state_ptr, __ATOMIC_RELAXED);

    int futex_op = FUTEX_WAKE;
    if (!(old_state & 1))
        futex_op |= FUTEX_PRIVATE_FLAG;

    __atomic_fetch_add(state_ptr, 4, __ATOMIC_RELAXED);

    syscall(SYS_futex, state_ptr, futex_op, 1, nullptr, nullptr, 0);
    return 0;
}

// POSIX-compatible strerror_r wrapper.
// Bionic's strerror_r is the POSIX form (returns int 0 on success); glibc
// with _GNU_SOURCE returns char*. This wrapper provides POSIX semantics.
int posix_strerror_r(int errnum, char* buf, size_t buflen) {
    char* result = strerror_r(errnum, buf, buflen);
    if (result && result != buf && buflen > 0) {
        strncpy(buf, result, buflen);
        buf[buflen - 1] = '\0';
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Bionic FILE* wrappers.
//
// libc-shim wraps glibc FILE* in a bionic-shaped struct where the real glibc
// FILE* lives at byte offset 24 (LP64). To call glibc's FILE* functions on a
// FILE* that came out of libPlayFab, we must unwrap first; for fopen() we
// have to construct a fresh wrapper around the glibc result.
// ---------------------------------------------------------------------------
struct bionic_file_wrapper {
    static FILE* unwrap(void* bf) {
        return bf ? *reinterpret_cast<FILE**>(reinterpret_cast<char*>(bf) + 24) : nullptr;
    }
    static void* wrap(FILE* f) {
        if (!f) return nullptr;
        char* bf = (char*)calloc(1, 152);
        *reinterpret_cast<const char**>(bf) = "plt_wrap";  // _p
        *reinterpret_cast<FILE**>(bf + 24) = f;            // wrapped
        *reinterpret_cast<int*>(bf + 20) = fileno(f);      // _file
        return bf;
    }
    static void* do_fopen(const char* path, const char* mode) {
        return wrap(::fopen(path, mode));
    }
    static int do_fclose(void* bf) {
        if (!bf) return EOF;
        int ret = ::fclose(unwrap(bf));
        ::free(bf);
        return ret;
    }
    static int do_fprintf(void* bf, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        int ret = ::vfprintf(unwrap(bf), fmt, ap);
        va_end(ap);
        return ret;
    }
    static int do_fputs(const char* s, void* bf) {
        return ::fputs(s, unwrap(bf));
    }
    static int do_fputc(int c, void* bf) {
        return ::fputc(c, unwrap(bf));
    }
    static int do_fgetc(void* bf) {
        return ::fgetc(unwrap(bf));
    }
    static size_t do_fread(void* ptr, size_t sz, size_t n, void* bf) {
        return ::fread(ptr, sz, n, unwrap(bf));
    }
    static size_t do_fwrite(const void* ptr, size_t sz, size_t n, void* bf) {
        return ::fwrite(ptr, sz, n, unwrap(bf));
    }
    static int do_fflush(void* bf) {
        return ::fflush(unwrap(bf));
    }
    static int do_fseek(void* bf, long off, int whence) {
        return ::fseek(unwrap(bf), off, whence);
    }
    static int do_ungetc(int c, void* bf) {
        return ::ungetc(c, unwrap(bf));
    }
    static int do_vfprintf(void* bf, const char* fmt, va_list ap) {
        return ::vfprintf(unwrap(bf), fmt, ap);
    }
    static long do_ftell(void* bf) {
        return ::ftell(unwrap(bf));
    }
};

// ---------------------------------------------------------------------------
// Sanitizing wrapper for libPlayFab's __emutls_get_address.
//
// PairIP encrypts the .data section, leaving emutls_control structs with
// garbage values. When __emutls_get_address reads a garbage index field,
// it computes an absurd realloc size and crashes. This wrapper detects
// corrupted controls (index > 100) and zero-initializes them before
// forwarding to the real implementation.
//
// emutls_control layout: [0]=size [1]=align [2]=index [3]=value
// ---------------------------------------------------------------------------
uintptr_t saved_real_emutls = 0;

void* sanitize_emutls_get_address(void* control) {
    uint64_t* c = (uint64_t*)control;
    if (c[2] > 100) {
        Log::info("Launcher",
            "PairIP: sanitizing emutls_control at %p (size=0x%lx align=0x%lx index=0x%lx value=0x%lx)",
            control, (unsigned long)c[0], (unsigned long)c[1],
            (unsigned long)c[2], (unsigned long)c[3]);
        c[0] = 1024;  // size (generous default for any TLS variable)
        c[1] = 8;     // align
        c[2] = 0;     // index = 0 (force fresh allocation at runtime)
        c[3] = 0;     // value = NULL (zero-initialize)
    }
    typedef void* (*emutls_fn_t)(void*);
    return ((emutls_fn_t)saved_real_emutls)(control);
}

// ---------------------------------------------------------------------------
// PLT resolver trampoline + handler.
//
// When an unknown PLT slot is invoked, the PLT fallback code pushes the slot
// index onto the stack and jumps through GOT[2] to our resolver. We try to
// look up the symbol via .rela.plt; if no relocation is found we install a
// no-op stub so the caller does not crash.
//
// Stack on entry to plt_resolver_trampoline:
//   [GOT[1] = soinfo_ctx pointer] [PLT_index] [return_addr]
// ---------------------------------------------------------------------------

// Per-library resolver context. We allocate one per fixed-up library and
// stash its pointer in GOT[1] so the resolver knows which library it is
// resolving for. (In bionic this slot holds a soinfo*; we synthesize our
// own minimal struct.)
struct ResolverContext {
    uintptr_t load_bias;            // = base address (mapping is at vaddr 0)
    const Elf64_Rela* plt_rela;
    size_t plt_rela_count;
    const Elf64_Sym* symtab;
    const char* strtab;
    Elf64_Addr* plt_got_addr;
    const char* path;
};

extern "C" Elf64_Addr mcpelauncher_pairip_plt_resolve_handler(void* ctx_ptr, long plt_index, void* return_addr);

__attribute__((naked)) void plt_resolver_trampoline() {
    asm volatile(
        // Save caller-saved registers.
        "sub $0x38, %%rsp\n"
        "mov %%rax, 0x00(%%rsp)\n"
        "mov %%rcx, 0x08(%%rsp)\n"
        "mov %%rdx, 0x10(%%rsp)\n"
        "mov %%rsi, 0x18(%%rsp)\n"
        "mov %%rdi, 0x20(%%rsp)\n"
        "mov %%r8,  0x28(%%rsp)\n"
        "mov %%r9,  0x30(%%rsp)\n"
        // Extract GOT[1] (resolver context*), PLT index, and return address.
        "mov 0x38(%%rsp), %%rdi\n"  // GOT[1] = ResolverContext*
        "mov 0x40(%%rsp), %%rsi\n"  // PLT index
        "mov 0x48(%%rsp), %%rdx\n"  // caller return address
        "call mcpelauncher_pairip_plt_resolve_handler\n"
        "mov %%rax, %%r11\n"        // Save resolved address.
        // Restore caller-saved registers.
        "mov 0x00(%%rsp), %%rax\n"
        "mov 0x08(%%rsp), %%rcx\n"
        "mov 0x10(%%rsp), %%rdx\n"
        "mov 0x18(%%rsp), %%rsi\n"
        "mov 0x20(%%rsp), %%rdi\n"
        "mov 0x28(%%rsp), %%r8\n"
        "mov 0x30(%%rsp), %%r9\n"
        // Pop save area (0x38) + GOT[1] (0x08) + PLT index (0x08) = 0x48.
        "add $0x48, %%rsp\n"
        // Jump to resolved function (return addr is now on top of stack).
        "jmp *%%r11\n"
        ::: "memory"
    );
}

extern "C" Elf64_Addr mcpelauncher_pairip_plt_resolve_handler(void* ctx_ptr, long plt_index, void* return_addr) {
    ResolverContext* ctx = reinterpret_cast<ResolverContext*>(ctx_ptr);
    if (!ctx) {
        fprintf(stderr, "FATAL: PairIP PLT resolver called with null context (index=%ld)\n", plt_index);
        abort();
    }

    // Compute the .got.plt entry address for this PLT index.
    Elf64_Addr* got_entry = ctx->plt_got_addr + plt_index + 3;
    Elf64_Addr got_entry_addr = reinterpret_cast<Elf64_Addr>(got_entry);

    // Search .rela.plt for a matching r_offset.
    if (ctx->plt_rela != nullptr && ctx->symtab != nullptr && ctx->strtab != nullptr) {
        for (size_t i = 0; i < ctx->plt_rela_count; i++) {
            Elf64_Addr r_offset_addr = ctx->load_bias + ctx->plt_rela[i].r_offset;
            if (r_offset_addr == got_entry_addr) {
                uint32_t sym_idx = ELF64_R_SYM(ctx->plt_rela[i].r_info);
                const char* sym_name = ctx->strtab + ctx->symtab[sym_idx].st_name;
                Log::info("Launcher", "PairIP PLT resolver: index=%ld -> symbol '%s'", plt_index, sym_name);

                const Elf64_Sym* local_sym = &ctx->symtab[sym_idx];
                if (local_sym->st_shndx != SHN_UNDEF) {
                    Elf64_Addr resolved = ctx->load_bias + local_sym->st_value;
                    *got_entry = resolved;
                    return resolved;
                }

                // Undefined symbol — would need to search loaded libraries.
                // Fall through to the no-op stub below.
                Log::warn("Launcher",
                    "PairIP PLT resolver cannot resolve undefined symbol '%s' (index=%ld) — using no-op",
                    sym_name, plt_index);
                break;
            }
        }
    }

    // No usable .rela.plt entry. Install no-op stub.
    Elf64_Addr caller_offset = reinterpret_cast<Elf64_Addr>(return_addr) - ctx->load_bias;
    Log::warn("Launcher",
        "PairIP: unresolved PLT entry %ld in %s (caller offset 0x%lx) — using no-op stub",
        plt_index, ctx->path ? ctx->path : "(libPlayFabMultiplayer.so)",
        (unsigned long)caller_offset);

    Elf64_Addr noop_addr = reinterpret_cast<Elf64_Addr>(unrelocated_plt_noop);
    *got_entry = noop_addr;
    return noop_addr;
}

// ---------------------------------------------------------------------------
// In-memory ELF parsing for libPlayFabMultiplayer.so.
// We do not have access to bionic's soinfo from the launcher; the library is
// already mapped, so we walk the program headers and dynamic table ourselves.
// The launcher mapping uses load_bias = base (vaddr 0).
// ---------------------------------------------------------------------------
struct PfmElf {
    uintptr_t base = 0;             // load bias / mapped base
    Elf64_Addr* plt_got = nullptr;  // DT_PLTGOT
    const Elf64_Rela* plt_rela = nullptr;
    size_t plt_rela_count = 0;
    const Elf64_Sym* symtab = nullptr;
    const char* strtab = nullptr;
    const Elf64_Phdr* phdrs = nullptr;
    Elf64_Half phnum = 0;
};

bool parse_pfm_elf(uintptr_t base, PfmElf& out) {
    out.base = base;
    auto* eh = reinterpret_cast<const Elf64_Ehdr*>(base);
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) {
        Log::warn("Launcher", "PairIP: libPlayFabMultiplayer.so does not look like ELF at %p",
                  reinterpret_cast<void*>(base));
        return false;
    }
    out.phdrs = reinterpret_cast<const Elf64_Phdr*>(base + eh->e_phoff);
    out.phnum = eh->e_phnum;

    const Elf64_Dyn* dyn = nullptr;
    for (Elf64_Half i = 0; i < eh->e_phnum; i++) {
        if (out.phdrs[i].p_type == PT_DYNAMIC) {
            dyn = reinterpret_cast<const Elf64_Dyn*>(base + out.phdrs[i].p_vaddr);
            break;
        }
    }
    if (!dyn) {
        Log::warn("Launcher", "PairIP: libPlayFabMultiplayer.so has no PT_DYNAMIC");
        return false;
    }

    Elf64_Xword pltrelsz = 0;
    for (const Elf64_Dyn* d = dyn; d->d_tag != DT_NULL; ++d) {
        switch (d->d_tag) {
            case DT_PLTGOT:
                out.plt_got = reinterpret_cast<Elf64_Addr*>(base + d->d_un.d_ptr);
                break;
            case DT_JMPREL:
                out.plt_rela = reinterpret_cast<const Elf64_Rela*>(base + d->d_un.d_ptr);
                break;
            case DT_PLTRELSZ:
                pltrelsz = d->d_un.d_val;
                break;
            case DT_SYMTAB:
                out.symtab = reinterpret_cast<const Elf64_Sym*>(base + d->d_un.d_ptr);
                break;
            case DT_STRTAB:
                out.strtab = reinterpret_cast<const char*>(base + d->d_un.d_ptr);
                break;
            default:
                break;
        }
    }
    if (out.plt_rela && pltrelsz)
        out.plt_rela_count = pltrelsz / sizeof(Elf64_Rela);
    return out.plt_got != nullptr;
}

} // namespace

namespace mcpelauncher {

void apply_pairip_plt_workaround() {
    // Look up the existing handle without triggering a fresh load. If the
    // library was not pulled in by libminecraftpe.so, we have nothing to do.
    void* pfm = linker::dlopen("libPlayFabMultiplayer.so", RTLD_NOLOAD);
    if (!pfm) {
        Log::info("Launcher", "PairIP: libPlayFabMultiplayer.so not loaded — skipping PLT workaround");
        return;
    }
    size_t pfm_base = linker::get_library_base(pfm);
    if (!pfm_base) {
        Log::warn("Launcher", "PairIP: failed to get base of libPlayFabMultiplayer.so");
        return;
    }
    Log::info("Launcher", "PairIP: libPlayFabMultiplayer.so loaded at 0x%lx", (unsigned long)pfm_base);

    PfmElf pfm_elf;
    if (!parse_pfm_elf(pfm_base, pfm_elf))
        return;

    Elf64_Addr* plt_got_addr_ = pfm_elf.plt_got;
    const Elf64_Rela* plt_rela_ = pfm_elf.plt_rela;
    size_t plt_rela_count_ = pfm_elf.plt_rela_count;
    uintptr_t load_bias = pfm_elf.base;
    const char* realpath = "libPlayFabMultiplayer.so";

    if (plt_got_addr_ == nullptr || plt_rela_ == nullptr || plt_rela_count_ == 0) {
        Log::info("Launcher", "PairIP: libPlayFab has no PLT/.rela.plt — skipping");
        return;
    }

    // The .got.plt page is normally read-only after relro. Make it writable
    // for the duration of the patch. We compute a generous span so the entire
    // .got.plt region (including GOT[1]/GOT[2] and the function entries we
    // touch below) is covered.
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    // Build set of .got.plt addresses that were resolved by .rela.plt.
    std::unordered_set<Elf64_Addr> resolved_addrs;
    for (size_t i = 0; i < plt_rela_count_; i++) {
        resolved_addrs.insert(load_bias + plt_rela_[i].r_offset);
    }

    // PLT entries not listed in the per-version table fall through to the
    // resolver trampoline, which uses a no-op stub for unresolvable entries.

    struct plt_func_entry { size_t plt_idx; void* func; const char* name; };

    // ========================================================================
    // v1.26.x libPlayFabMultiplayer.so (205 PLT entries, plt_base=0x3d7190)
    // ========================================================================
    static const plt_func_entry known_plt_funcs_v126[] = {
      // Memory operations
      { 4, (void*)memset, "memset" },
      { 5, (void*)memcpy, "memcpy" },
      { 11, (void*)memmove, "memmove" },
      { 12, (void*)memcmp, "memcmp" },
      { 66, (void*)malloc, "malloc" },
      { 69, (void*)calloc, "calloc" },
      { 108, (void*)realloc, "realloc" },
      // File I/O (wrapped: bionic FILE* -> glibc FILE* at offset 24)
      { 79, (void*)bionic_file_wrapper::do_fprintf, "fprintf" },
      { 145, (void*)bionic_file_wrapper::do_fwrite, "fwrite" },
      { 148, (void*)bionic_file_wrapper::do_fflush, "fflush" },
      { 192, (void*)bionic_file_wrapper::do_fputc, "fputc" },
      // Threading
      { 109, (void*)pthread_mutex_lock, "pthread_mutex_lock" },
      { 110, (void*)pthread_mutex_unlock, "pthread_mutex_unlock" },
      { 130, (void*)bionic_pthread_cond_signal, "pthread_cond_signal" },
      // Locale
      { 154, (void*)newlocale, "newlocale" },
      { 155, (void*)uselocale, "uselocale" },
      { 161, (void*)freelocale, "freelocale" },
      // String formatting
      { 157, (void*)snprintf, "snprintf" },
      // String-to-number conversions (errno save/clear/check pattern)
      { 115, (void*)strtol, "strtol" },
      { 116, (void*)strtoll, "strtoll" },
      { 117, (void*)strtod, "strtod" },
      { 118, (void*)wcstol, "wcstol" },
      { 119, (void*)wcstoul, "wcstoul" },
      { 120, (void*)wcstol, "wcstol" },
      { 121, (void*)wcstod, "wcstod" },
      { 122, (void*)wcstod, "wcstod" },
      { 123, (void*)wcstold, "wcstold" },
      { 124, (void*)snprintf, "snprintf" },
      { 125, (void*)strtoul, "strtoul" },
      { 126, (void*)wcstoul, "wcstoul" },
      // Multibyte/wide char conversion
      { 113, (void*)mbrtowc, "mbrtowc" },
      // System
      { 190, (void*)syscall, "syscall" },
    };
    static const size_t known_plt_funcs_v126_count = sizeof(known_plt_funcs_v126) / sizeof(known_plt_funcs_v126[0]);

    // ========================================================================
    // v1.21.x libPlayFabMultiplayer.so (523 PLT entries, plt_base=0x408ed0)
    // ========================================================================
    static const plt_func_entry known_plt_funcs[] = {
      // Memory operations
      { 4, (void*)memset, "memset" },
      { 5, (void*)memmove, "memmove" },
      { 9, (void*)free, "free" },
      { 27, (void*)memcpy, "memcpy" },
      { 35, (void*)strlen, "strlen" },
      { 64, (void*)strcmp, "strcmp" },
      { 97, (void*)snprintf, "snprintf" },
      { 103, (void*)malloc, "malloc" },
      { 134, (void*)calloc, "calloc" },
      { 232, (void*)realloc, "realloc" },
      { 512, (void*)posix_memalign, "posix_memalign" },
      // String formatting
      { 47, (void*)sprintf, "sprintf" },
      { 126, (void*)bionic_file_wrapper::do_fprintf, "fprintf" },
      { 274, (void*)snprintf, "snprintf" },
      // String-to-number conversions
      { 41, (void*)strtoull, "strtoull" },
      { 42, (void*)strtoll, "strtoll" },
      { 43, (void*)strtod, "strtod" },
      { 265, (void*)strtoul, "strtoul" },
      { 266, (void*)strtof, "strtof" },
      { 267, (void*)strtold, "strtold" },
      // Wide string operations
      { 146, (void*)swprintf, "swprintf" },
      { 167, (void*)wcslen, "wcslen" },
      { 258, (void*)wmemchr, "wmemchr" },
      { 263, (void*)wmemcmp, "wmemcmp" },
      { 268, (void*)wcstoul, "wcstoul" },
      { 269, (void*)wcstoll, "wcstoll" },
      { 270, (void*)wcstoull, "wcstoull" },
      { 271, (void*)wcstof, "wcstof" },
      { 272, (void*)wcstod, "wcstod" },
      { 273, (void*)wcstold, "wcstold" },
      // File I/O (wrapped: bionic FILE* -> glibc FILE* at offset 24)
      { 98, (void*)bionic_file_wrapper::do_fputs, "fputs" },
      { 99, (void*)bionic_file_wrapper::do_fopen, "fopen" },
      { 100, (void*)bionic_file_wrapper::do_fseek, "fseek" },
      { 101, (void*)bionic_file_wrapper::do_ftell, "ftell" },
      { 102, (void*)bionic_file_wrapper::do_fseek, "fseek" },
      { 104, (void*)bionic_file_wrapper::do_fread, "fread" },
      { 106, (void*)bionic_file_wrapper::do_fclose, "fclose" },
      { 345, (void*)bionic_file_wrapper::do_fwrite, "fwrite" },
      { 346, (void*)bionic_file_wrapper::do_fflush, "fflush" },
      { 348, (void*)bionic_file_wrapper::do_fseek, "fseek" },
      { 349, (void*)bionic_file_wrapper::do_ungetc, "ungetc" },
      { 350, (void*)bionic_file_wrapper::do_fgetc, "fgetc" },
      { 351, (void*)bionic_file_wrapper::do_ungetc, "ungetc" },
      { 352, (void*)bionic_file_wrapper::do_fgetc, "fgetc" },
      { 353, (void*)bionic_file_wrapper::do_fputc, "fputc" },
      { 505, (void*)bionic_file_wrapper::do_vfprintf, "vfprintf" },
      { 506, (void*)bionic_file_wrapper::do_fputc, "fputc" },
      { 507, (void*)free, "free" },
      // abort_message helpers
      { 508, (void*)strlen, "strlen" },
      { 509, (void*)dprintf, "dprintf" },
      { 510, (void*)abort, "abort" },
      // Exception handling mutex
      { 519, (void*)pthread_mutex_lock, "pthread_mutex_lock" },
      { 520, (void*)pthread_mutex_unlock, "pthread_mutex_unlock" },
      { 522, (void*)pthread_mutex_lock, "pthread_mutex_lock" },
      // Thread creation
      { 119, (void*)pthread_create, "pthread_create" },
      // Event fd (task queue signaling)
      { 121, (void*)eventfd, "eventfd" },
      // File descriptor operations
      { 138, (void*)close, "close" },
      { 139, (void*)write, "write" },
      { 141, (void*)read, "read" },
      // UUID formatting
      { 145, (void*)snprintf, "snprintf" },
      // Thread-local storage support
      { 128, (void*)pthread_setspecific, "pthread_setspecific" },
      { 292, (void*)pthread_getspecific, "pthread_getspecific" },
      { 312, (void*)pthread_key_create, "pthread_key_create" },
      { 517, (void*)pthread_once, "pthread_once" },
      // Time
      { 110, (void*)localtime, "localtime" },
      { 111, (void*)strftime, "strftime" },
      { 161, (void*)clock_gettime, "clock_gettime" },
      // Mutex (glibc-compatible for NORMAL type on x86_64 LE)
      { 246, (void*)pthread_mutex_lock, "pthread_mutex_lock" },
      { 247, (void*)pthread_mutex_unlock, "pthread_mutex_unlock" },
      { 302, (void*)pthread_mutex_destroy, "pthread_mutex_destroy" },
      { 303, (void*)pthread_mutex_trylock, "pthread_mutex_trylock" },
      { 304, (void*)pthread_mutexattr_init, "pthread_mutexattr_init" },
      { 305, (void*)pthread_mutexattr_settype, "pthread_mutexattr_settype" },
      { 306, (void*)pthread_mutex_init, "pthread_mutex_init" },
      { 307, (void*)pthread_mutexattr_destroy, "pthread_mutexattr_destroy" },
      // Threading
      { 308, (void*)pthread_join, "pthread_join" },
      { 309, (void*)pthread_detach, "pthread_detach" },
      { 311, (void*)nanosleep, "nanosleep" },
      // Condition variables (bionic-compatible, use futex directly)
      { 287, (void*)bionic_pthread_cond_signal, "pthread_cond_destroy" },
      { 288, (void*)bionic_pthread_cond_signal, "pthread_cond_signal" },
      { 289, (void*)bionic_pthread_cond_broadcast, "pthread_cond_broadcast" },
      { 290, (void*)bionic_pthread_cond_wait, "pthread_cond_wait" },
      { 291, (void*)bionic_pthread_cond_timedwait, "pthread_cond_timedwait" },
      // Error handling
      { 284, (void*)posix_strerror_r, "strerror_r" },
      { 285, (void*)abort, "abort" },
      // Locale basics
      { 17, (void*)localeconv, "localeconv" },
      { 310, (void*)sysconf, "sysconf" },
      { 360, (void*)newlocale, "newlocale" },
      { 361, (void*)uselocale, "uselocale" },
      { 376, (void*)strftime, "strftime" },
      { 385, (void*)wcstombs, "wcstombs" },
      { 417, (void*)setlocale, "setlocale" },
      { 419, (void*)freelocale, "freelocale" },
      // Locale-aware collation (from collate_byname)
      { 421, (void*)strcoll_l, "strcoll_l" },
      { 422, (void*)strxfrm_l, "strxfrm_l" },
      { 424, (void*)wcscoll_l, "wcscoll_l" },
      { 425, (void*)wcsxfrm_l, "wcsxfrm_l" },
      // Locale-aware wide char classification (from ctype_byname<wchar_t>)
      { 427, (void*)iswalpha_l, "iswalpha_l" },
      { 437, (void*)iswspace_l, "iswspace_l" },
      { 438, (void*)iswprint_l, "iswprint_l" },
      { 439, (void*)iswcntrl_l, "iswcntrl_l" },
      { 440, (void*)iswupper_l, "iswupper_l" },
      { 441, (void*)iswlower_l, "iswlower_l" },
      { 442, (void*)iswdigit_l, "iswdigit_l" },
      { 443, (void*)iswpunct_l, "iswpunct_l" },
      { 444, (void*)iswxdigit_l, "iswxdigit_l" },
      { 445, (void*)iswblank_l, "iswblank_l" },
      // Locale-aware wide char case conversion (from ctype_byname<wchar_t>)
      { 446, (void*)towupper_l, "towupper_l" },
      { 447, (void*)towlower_l, "towlower_l" },
      // Wide char byte conversion (from ctype_byname<wchar_t> widen/narrow)
      { 448, (void*)btowc, "btowc" },
      { 449, (void*)wctob, "wctob" },
      // Multibyte/wide char conversion (from codecvt/moneypunct/time_get)
      { 378, (void*)mbsrtowcs, "mbsrtowcs" },
      { 452, (void*)wcrtomb, "wcrtomb" },
      { 453, (void*)wcrtomb, "wcrtomb" },
      { 454, (void*)mbrtowc, "mbrtowc" },
      { 455, (void*)mbrtowc, "mbrtowc" },
      { 456, (void*)mbtowc, "mbtowc" },
      { 458, (void*)mbrlen, "mbrlen" },
      // Locale-aware number parsing (from num_get, near __errno)
      { 486, (void*)strtoll_l, "strtoll_l" },
      { 487, (void*)strtoull_l, "strtoull_l" },
      // System
      { 491, (void*)syscall, "syscall" },
    };
    static const size_t known_plt_funcs_count = sizeof(known_plt_funcs) / sizeof(known_plt_funcs[0]);

    Elf64_Addr* func_entries = plt_got_addr_ + 3; // Skip 3 reserved entries.
    size_t fixed_count = 0;
    size_t known_count = 0;
    size_t plt_count = 0;

    // Determine PLT count by finding the PLT section base address.
    // Unresolved .got.plt entries contain raw PLT fallback addresses:
    //   value = PLT_base + 0x10 + index * 0x10 + 6 = PLT_base + 0x16 + index * 0x10
    // We find the first unresolved entry, derive PLT_base, then validate all
    // subsequent entries.
    Elf64_Addr plt_section_base = 0;
    for (size_t i = 0; i < 2048; i++) {
        Elf64_Addr entry_addr = reinterpret_cast<Elf64_Addr>(&func_entries[i]);
        Elf64_Addr value = func_entries[i];

        if (value == 0) break;

        if (resolved_addrs.count(entry_addr)) {
            // Resolved by .rela.plt - valid .got.plt entry.
            plt_count = i + 1;
            continue;
        }

        if (value > 0 && value < load_bias) {
            if (plt_section_base == 0) {
                plt_section_base = value - 0x16 - i * 0x10;
            }
            Elf64_Addr expected = plt_section_base + 0x16 + i * 0x10;
            if (value != expected) {
                break;
            }
            plt_count = i + 1;
        } else {
            break;
        }
    }

    if (plt_section_base == 0) {
        Log::info("Launcher", "PairIP: no unresolved PLT entries in libPlayFabMultiplayer.so — skipping");
        return;
    }

    Log::info("Launcher",
        "PairIP PLT fixup: %zu PLT entries (plt_base=0x%lx) in %s",
        plt_count, (unsigned long)plt_section_base, realpath);

    // Make .got.plt writable. We mprotect a page span covering plt_got_addr
    // through the highest entry index we will touch, plus the entries we
    // patch as part of the emutls hook below.
    auto mprotect_span = [page_size](void* lo_addr, size_t bytes, int prot) -> bool {
        uintptr_t lo = reinterpret_cast<uintptr_t>(lo_addr);
        uintptr_t hi = lo + bytes;
        uintptr_t page_lo = lo & ~static_cast<uintptr_t>(page_size - 1);
        uintptr_t page_hi = (hi + page_size - 1) & ~static_cast<uintptr_t>(page_size - 1);
        return mprotect(reinterpret_cast<void*>(page_lo), page_hi - page_lo, prot) == 0;
    };

    // Coverage: from plt_got_addr_[0] through func_entries[plt_count - 1].
    size_t got_bytes = (plt_count + 3) * sizeof(Elf64_Addr);
    if (!mprotect_span(plt_got_addr_, got_bytes, PROT_READ | PROT_WRITE)) {
        Log::warn("Launcher", "PairIP: mprotect(.got.plt) failed: %s", strerror(errno));
        return;
    }

    {
    const plt_func_entry* active_table = nullptr;
    size_t active_table_count = 0;

    if (plt_count > 500) {
        // v1.21.x libPlayFabMultiplayer.so (523 PLT entries).
        active_table = known_plt_funcs;
        active_table_count = known_plt_funcs_count;
    } else if (plt_count >= 200 && plt_count <= 210) {
        // v1.26.x libPlayFabMultiplayer.so (205 PLT entries).
        active_table = known_plt_funcs_v126;
        active_table_count = known_plt_funcs_v126_count;
    } else {
        Log::warn("Launcher",
            "PairIP: unrecognized PLT count %zu in libPlayFab — only resolver fallback will be used",
            plt_count);
    }

    // Patch the entries.
    for (size_t i = 0; i < plt_count; i++) {
        Elf64_Addr entry_addr = reinterpret_cast<Elf64_Addr>(&func_entries[i]);

        // Skip entries already resolved by .rela.plt.
        if (resolved_addrs.count(entry_addr)) continue;

        Elf64_Addr value = func_entries[i];
        if (value > 0 && value < load_bias) {
            bool found = false;
            if (active_table) {
                for (size_t j = 0; j < active_table_count; j++) {
                    if (i == active_table[j].plt_idx) {
                        func_entries[i] = reinterpret_cast<Elf64_Addr>(active_table[j].func);
                        known_count++;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // Unknown entry — apply load_bias so PLT fallback triggers
                // resolver via GOT[2].
                func_entries[i] = value + load_bias;
                fixed_count++;
            }
        }
    }

    if (fixed_count > 0 || known_count > 0) {
        Log::info("Launcher",
            "PairIP PLT fixup in %s: %zu resolved to known funcs, %zu using resolver",
            realpath, known_count, fixed_count);

        // Allocate persistent resolver context (leaked intentionally — single
        // per-process, lives for the lifetime of the launcher).
        static ResolverContext s_ctx;
        s_ctx.load_bias = load_bias;
        s_ctx.plt_rela = plt_rela_;
        s_ctx.plt_rela_count = plt_rela_count_;
        s_ctx.symtab = pfm_elf.symtab;
        s_ctx.strtab = pfm_elf.strtab;
        s_ctx.plt_got_addr = plt_got_addr_;
        s_ctx.path = realpath;

        // GOT[1] = resolver context pointer.
        plt_got_addr_[1] = reinterpret_cast<Elf64_Addr>(&s_ctx);
        // GOT[2] = PLT resolver trampoline.
        plt_got_addr_[2] = reinterpret_cast<Elf64_Addr>(plt_resolver_trampoline);
    }
    } // end version-specific table scope

    // ------------------------------------------------------------------
    // emutls fixups for libPlayFabMultiplayer.so v1.21.x.
    //
    // PairIP-encrypted emutls_control structs in .data plus a hook on the
    // local __emutls_get_address. The VAs and code patch are specific to
    // the v1.21.x build.
    // ------------------------------------------------------------------
    if (plt_count > 500) {
        // Step 1: Patch known emutls_control structs directly in .data.
        // emutls_control layout: [0]=size [1]=align [2]=index [3]=value
        // (each 8 bytes).
        static const struct { size_t va; size_t size; } emutls_controls[] = {
            { 0x41fc68, 256 },   // PubSub TLS var 1
            { 0x41fc88, 256 },   // PubSub TLS var 2
            { 0x41fcf8, 256 },   // HttpClient TLS var
            { 0x41fd38, 256 },   // AndroidJniHelper TLS var 1
            { 0x41fd58, 256 },   // AndroidJniHelper TLS var 2
            { 0x41fd78, 256 },   // AndroidJniHelper TLS var 3
            { 0x41ffc0, 128 },   // __cxa_eh_globals (C++ exception handling)
        };
        for (const auto& ec : emutls_controls) {
            uint64_t* control = reinterpret_cast<uint64_t*>(load_bias + ec.va);
            if (!mprotect_span(control, 4 * sizeof(uint64_t), PROT_READ | PROT_WRITE)) {
                Log::warn("Launcher",
                    "PairIP: mprotect(emutls VA 0x%lx) failed: %s",
                    (unsigned long)ec.va, strerror(errno));
                continue;
            }
            uint64_t old_index = control[2];
            control[0] = ec.size;
            control[1] = 8;
            control[2] = 0;
            control[3] = 0;
            Log::info("Launcher",
                "PairIP: fixed emutls_control at VA 0x%lx (old_index=0x%lx, size=%zu)",
                (unsigned long)ec.va, (unsigned long)old_index, ec.size);
        }

        // Step 2: Hook __emutls_get_address via libPlayFab's GOT.
        saved_real_emutls = load_bias + 0x404a70;
        Elf64_Addr* got_emutls = reinterpret_cast<Elf64_Addr*>(load_bias + 0x41adb8);
        if (mprotect_span(got_emutls, sizeof(Elf64_Addr), PROT_READ | PROT_WRITE)) {
            *got_emutls = reinterpret_cast<Elf64_Addr>(sanitize_emutls_get_address);
            Log::info("Launcher",
                "PairIP: hooked __emutls_get_address GOT@0x41adb8 -> sanitizer (real=%p)",
                (void*)saved_real_emutls);
        } else {
            Log::warn("Launcher",
                "PairIP: mprotect(emutls GOT) failed: %s", strerror(errno));
        }

        // Step 3: Patch __emutls_get_address function code with a trampoline.
        // The GOT hook only catches calls through libPlayFab's PLT.
        // libminecraftpe.so calls __emutls_get_address via global symbol
        // interposition through ITS OWN GOT, which we cannot rewrite. By
        // patching the function entry itself we catch all callers.
        //
        // Overwrite first 12 bytes of __emutls_get_address:
        //   41 57 41 56 41 55 41 54 53 48 89 fb
        //   (push r15/r14/r13/r12/rbx; mov rbx,rdi)
        // with a jump to an mmap'd trampoline that:
        //   1. Checks if control->index ([rdi+0x10]) > 100 (PairIP-encrypted)
        //   2. If so, fixes the struct (size=1024, align=8, index=0, value=0)
        //   3. Executes the overwritten instructions
        //   4. Jumps back to VA 0x404a7c (instruction after the patch)
        {
            void* trampoline = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (trampoline == MAP_FAILED) {
                Log::warn("Launcher",
                    "PairIP: mmap trampoline page for emutls code patch failed: %s",
                    strerror(errno));
            } else {
                unsigned char* t = reinterpret_cast<unsigned char*>(trampoline);
                size_t p = 0;

                // cmp qword [rdi+0x10], 100   (check control->index)
                t[p++] = 0x48; t[p++] = 0x83; t[p++] = 0x7f; t[p++] = 0x10; t[p++] = 0x64;
                // jbe skip_fix  (skip 31 bytes if index <= 100)
                t[p++] = 0x76; t[p++] = 0x1f;
                // mov qword [rdi], 0x400      (size = 1024)
                t[p++] = 0x48; t[p++] = 0xc7; t[p++] = 0x07;
                t[p++] = 0x00; t[p++] = 0x04; t[p++] = 0x00; t[p++] = 0x00;
                // mov qword [rdi+8], 8        (align = 8)
                t[p++] = 0x48; t[p++] = 0xc7; t[p++] = 0x47; t[p++] = 0x08;
                t[p++] = 0x08; t[p++] = 0x00; t[p++] = 0x00; t[p++] = 0x00;
                // mov qword [rdi+0x10], 0     (index = 0)
                t[p++] = 0x48; t[p++] = 0xc7; t[p++] = 0x47; t[p++] = 0x10;
                t[p++] = 0x00; t[p++] = 0x00; t[p++] = 0x00; t[p++] = 0x00;
                // mov qword [rdi+0x18], 0     (value = NULL)
                t[p++] = 0x48; t[p++] = 0xc7; t[p++] = 0x47; t[p++] = 0x18;
                t[p++] = 0x00; t[p++] = 0x00; t[p++] = 0x00; t[p++] = 0x00;

                // skip_fix: Execute the overwritten instructions.
                t[p++] = 0x41; t[p++] = 0x57;  // push r15
                t[p++] = 0x41; t[p++] = 0x56;  // push r14
                t[p++] = 0x41; t[p++] = 0x55;  // push r13
                t[p++] = 0x41; t[p++] = 0x54;  // push r12
                t[p++] = 0x53;                  // push rbx
                t[p++] = 0x48; t[p++] = 0x89; t[p++] = 0xfb;  // mov rbx, rdi

                // movabs rax, <resume_addr>   (load_bias + 0x404a7c)
                uintptr_t resume_addr = load_bias + 0x404a7c;
                t[p++] = 0x48; t[p++] = 0xb8;
                memcpy(&t[p], &resume_addr, 8); p += 8;
                // jmp rax
                t[p++] = 0xff; t[p++] = 0xe0;

                // Patch the function entry.
                uintptr_t func_addr = load_bias + 0x404a70;
                if (!mprotect_span(reinterpret_cast<void*>(func_addr), 12,
                                   PROT_READ | PROT_WRITE | PROT_EXEC)) {
                    Log::warn("Launcher",
                        "PairIP: mprotect __emutls_get_address page failed: %s",
                        strerror(errno));
                } else {
                    unsigned char* func = reinterpret_cast<unsigned char*>(func_addr);
                    // movabs rax, <trampoline_addr>
                    func[0] = 0x48; func[1] = 0xb8;
                    uintptr_t tramp_addr = reinterpret_cast<uintptr_t>(trampoline);
                    memcpy(&func[2], &tramp_addr, 8);
                    // jmp rax
                    func[10] = 0xff; func[11] = 0xe0;

                    mprotect_span(reinterpret_cast<void*>(func_addr), 12,
                                  PROT_READ | PROT_EXEC);

                    Log::info("Launcher",
                        "PairIP: patched __emutls_get_address @0x%lx -> trampoline @%p (resume @0x%lx, %zu bytes)",
                        (unsigned long)func_addr, trampoline, (unsigned long)resume_addr, p);
                }
            }
        }
    }

    // Restore .got.plt protection. We made it RW above; put it back to RO.
    mprotect_span(plt_got_addr_, got_bytes, PROT_READ);
}

} // namespace mcpelauncher

#else  // !defined(__x86_64__)

namespace mcpelauncher {

void apply_pairip_plt_workaround() {
    // No-op on non-x86_64 architectures. The Android x86_64 PairIP build is
    // the only target where libPlayFabMultiplayer.so PLT scrambling occurs.
}

} // namespace mcpelauncher

#endif // __x86_64__
