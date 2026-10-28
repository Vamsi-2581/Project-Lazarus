// Stand-in for UnityPlayer.dll. What matters is the import table: it pulls
// VERSION.dll in statically (so our proxy gets loaded, same as the real
// engine) and resolves il2cpp_* through its own GetProcAddress import.

#include <windows.h>

#include <cstdio>

#pragma comment(lib, "version.lib")

using init_t = int (*)(const char*);
using shutdown_t = void (*)();
using load_scene_t = void (*)(int, int);
using get_value_t = int (*)();
using state_t = int (*)(const char*);

// Writes key=value lines to `report` so the test runner can check them.
extern "C" __declspec(dllexport) int fake_unity_main(const wchar_t* report_path) {
    FILE* r = _wfopen(report_path, L"w");
    if (!r) return 90;

    // forwarding check: this goes through our proxy's export stub
    DWORD dummy = 0;
    DWORD vsize = GetFileVersionInfoSizeW(L"kernel32.dll", &dummy);
    std::fprintf(r, "version_forward=%d\n", vsize > 0 ? 1 : 0);

    HMODULE ga = LoadLibraryW(L"GameAssembly.dll");
    if (!ga) return 91;
    auto init = reinterpret_cast<init_t>(GetProcAddress(ga, "il2cpp_init"));
    auto shutdown = reinterpret_cast<shutdown_t>(GetProcAddress(ga, "il2cpp_shutdown"));
    auto load_scene = reinterpret_cast<load_scene_t>(GetProcAddress(ga, "fake_engine_load_scene"));
    auto get_value = reinterpret_cast<get_value_t>(GetProcAddress(ga, "fake_game_get_value"));
    auto state = reinterpret_cast<state_t>(GetProcAddress(ga, "fake_state"));
    if (!init || !shutdown || !load_scene || !get_value || !state) return 92;

    // if the proxy did its job, the il2cpp_init we got lives in version.dll,
    // not GameAssembly.dll
    HMODULE owner = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(init), &owner);
    wchar_t owner_name[MAX_PATH] = L"?";
    if (owner) GetModuleFileNameW(owner, owner_name, MAX_PATH);
    const wchar_t* base = wcsrchr(owner_name, L'\\');
    std::fprintf(r, "init_owner=%ls\n", base ? base + 1 : owner_name);
    std::fflush(r);

    int rc = init("IL2CPP Root Domain");
    std::fprintf(r, "init_rc=%d\nreal_init_called=%d\n", rc, state("init"));
    std::fflush(r);

    std::fprintf(r, "value_before_scene=%d\n", get_value());
    load_scene(1, 0);  // MainMenu, single
    load_scene(2, 1);  // Prison, additive
    std::fprintf(r, "scene_listener_calls=%d\n", state("scene_listener"));
    std::fprintf(r, "value_in_game=%d\n", get_value());
    std::fprintf(r, "target_frame_rate=%d\n", state("target_frame_rate"));
    std::fflush(r);

    shutdown();
    std::fprintf(r, "real_shutdown_called=%d\n", state("shutdown"));
    std::fprintf(r, "value_after_shutdown=%d\n", get_value());
    std::fprintf(r, "finished=1\n");
    std::fclose(r);
    return 0;
}
