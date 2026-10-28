// version.dll proxy - the only file Lazarus adds next to the game exe.
//
// Boot sequence:
//   1. UnityPlayer.dll statically imports VERSION.dll, Windows finds ours in
//      the game folder first (version.dll isn't a KnownDLL).
//   2. DllMain: load the real System32 version.dll for forwarding, and patch
//      UnityPlayer's IAT slot for GetProcAddress. That's it. Nothing heavy
//      under the loader lock.
//   3. UnityPlayer later LoadLibrary's GameAssembly.dll and pulls il2cpp_*
//      functions through GetProcAddress. We hand back our own il2cpp_init and
//      il2cpp_shutdown wrappers.
//   4. Our il2cpp_init calls the real one, then starts the runtime (runtime.cpp)
//      on the main thread with a live IL2CPP domain.

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "proxy/iat.h"
#include "proxy/runtime.h"

extern "C" void* lz_real_version[17] = {};

namespace {

const char* const kVersionExports[17] = {
    "GetFileVersionInfoA",     "GetFileVersionInfoByHandle", "GetFileVersionInfoExA",   "GetFileVersionInfoExW",
    "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExA",  "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW",
    "GetFileVersionInfoW",     "VerFindFileA",               "VerFindFileW",            "VerInstallFileA",
    "VerInstallFileW",         "VerLanguageNameA",           "VerLanguageNameW",        "VerQueryValueA",
    "VerQueryValueW",
};

// every version.dll export returns BOOL/DWORD, so "0 + last error" is a sane
// failure for anything we couldn't resolve. beats jumping to null.
extern "C" ULONG_PTR lz_missing_export() {
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

bool forward_init() {
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (!n || n > MAX_PATH - 16) return false;
    wcscat_s(path, L"\\version.dll");
    // LoadLibrary inside DllMain is officially frowned on, but System32
    // version.dll only depends on kernel32/ntdll which are already loaded,
    // so there's no lock-order problem here. every proxy DLL does this.
    HMODULE real = LoadLibraryW(path);
    for (int i = 0; i < 17; ++i) {
        void* p = real ? reinterpret_cast<void*>(GetProcAddress(real, kVersionExports[i])) : nullptr;
        lz_real_version[i] = p ? p : reinterpret_cast<void*>(&lz_missing_export);
    }
    return real != nullptr;
}

using GetProcAddress_t = FARPROC(WINAPI*)(HMODULE, LPCSTR);
using il2cpp_init_t = int (*)(const char*);
using il2cpp_shutdown_t = void (*)();

il2cpp_init_t g_real_init = nullptr;
il2cpp_shutdown_t g_real_shutdown = nullptr;
HMODULE g_game_assembly = nullptr;

int lz_il2cpp_init(const char* domain_name) {
    int r = g_real_init(domain_name);
    lzrt::on_il2cpp_ready(g_game_assembly);
    return r;
}

void lz_il2cpp_shutdown() {
    lzrt::on_il2cpp_shutdown();
    g_real_shutdown();
}

FARPROC WINAPI hooked_GetProcAddress(HMODULE mod, LPCSTR name) {
    FARPROC real = GetProcAddress(mod, name);  // our own import, not the patched slot
    // ordinal lookups have a null high word, don't strcmp those
    if (!real || (reinterpret_cast<ULONG_PTR>(name) >> 16) == 0) return real;
    if (name[0] != 'i' || std::strncmp(name, "il2cpp_", 7) != 0) return real;

    if (std::strcmp(name, "il2cpp_init") == 0) {
        g_real_init = reinterpret_cast<il2cpp_init_t>(real);
        g_game_assembly = mod;
        lzrt::bootstrap_note("intercepted il2cpp_init");
        return reinterpret_cast<FARPROC>(&lz_il2cpp_init);
    }
    if (std::strcmp(name, "il2cpp_shutdown") == 0) {
        g_real_shutdown = reinterpret_cast<il2cpp_shutdown_t>(real);
        return reinterpret_cast<FARPROC>(&lz_il2cpp_shutdown);
    }
    return real;
}

void attach() {
    bool fwd = forward_init();
    lzrt::bootstrap_note(fwd ? "forwarding to system version.dll" : "FAILED to load system version.dll");

    HMODULE unity = GetModuleHandleW(L"UnityPlayer.dll");
    if (!unity) {
        // not a Unity process (or a very weird one). stay a plain version.dll.
        lzrt::bootstrap_note("UnityPlayer.dll not loaded in this process, staying inert");
        return;
    }
    void* orig = nullptr;
    if (lzrt::patch_iat(unity, "KERNEL32.dll", "GetProcAddress", reinterpret_cast<void*>(&hooked_GetProcAddress), &orig))
        lzrt::bootstrap_note("patched UnityPlayer.dll GetProcAddress import");
    else
        lzrt::bootstrap_note("FAILED to patch UnityPlayer.dll GetProcAddress import - loader will not start");
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        lzrt::set_proxy_module(self);
        attach();
    }
    return TRUE;
}
