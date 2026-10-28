// Stand-in for the game's GameAssembly.dll. Implements just enough of the
// il2cpp C API for the loader to do its thing: two "assemblies", a
// SceneManager.Internal_SceneLoaded method, one fake gameplay method, and a
// few icalls. Exported fake_* functions let the fake engine drive it.

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <cwchar>

#define EXPORT extern "C" __declspec(dllexport)

namespace {

struct FakeMethodInfo {
    void* methodPointer;  // must be first, the loader relies on that layout
    const char* name;
    int argc;
};
struct FakeClass {
    const char* ns;
    const char* name;
    FakeMethodInfo* methods;
    int count;
};
struct FakeImage {
    const char* name;
    FakeClass* classes;
    int count;
};
struct FakeAssembly {
    FakeImage* image;
};
struct FakeString {
    const wchar_t* chars;
};

volatile int g_init_called = 0;
volatile int g_shutdown_called = 0;
volatile int g_game_scene_listener_calls = 0;
volatile int g_target_frame_rate = -1;

// noinline + distinct bodies so the linker can't fold or inline them away;
// MinHook needs a real function prologue to patch
__declspec(noinline) void Internal_SceneLoaded(int32_t handle, int32_t mode, const void*) {
    g_game_scene_listener_calls = g_game_scene_listener_calls + 1;
    if (handle < 0 || mode < 0) OutputDebugStringA("impossible");
}

__declspec(noinline) int FakeGame_GetValue(const void*) {
    volatile int v = 1;
    return v;
}

FakeMethodInfo g_scene_methods[] = {{(void*)&Internal_SceneLoaded, "Internal_SceneLoaded", 2}};
FakeMethodInfo g_game_methods[] = {{(void*)&FakeGame_GetValue, "GetValue", 0}};
FakeClass g_core_classes[] = {{"UnityEngine.SceneManagement", "SceneManager", g_scene_methods, 1}};
FakeClass g_game_classes[] = {{"Lucille.Fake", "FakeGame", g_game_methods, 1}};
FakeImage g_images[] = {{"UnityEngine.CoreModule.dll", g_core_classes, 1}, {"Lucille.Fake.dll", g_game_classes, 1}};
FakeAssembly g_assemblies[] = {{&g_images[0]}, {&g_images[1]}};
void* g_assembly_ptrs[] = {&g_assemblies[0], &g_assemblies[1]};
int g_domain = 0;

FakeString g_scene_names[] = {{L"Boot"}, {L"MainMenu"}, {L"Prison"}};

void* icall_GetNameInternal(int32_t handle) {
    return (handle >= 0 && handle < 3) ? &g_scene_names[handle] : nullptr;
}
int32_t icall_GetBuildIndexInternal(int32_t handle) { return handle; }
void icall_set_targetFrameRate(int32_t v) { g_target_frame_rate = v; }

}  // namespace

EXPORT int il2cpp_init(const char*) {
    g_init_called = 1;
    return 1;
}
EXPORT void il2cpp_shutdown() { g_shutdown_called = 1; }
EXPORT void* il2cpp_domain_get() { return &g_domain; }
EXPORT void** il2cpp_domain_get_assemblies(void*, size_t* size) {
    *size = 2;
    return g_assembly_ptrs;
}
EXPORT void* il2cpp_assembly_get_image(void* a) { return static_cast<FakeAssembly*>(a)->image; }
EXPORT const char* il2cpp_image_get_name(void* i) { return static_cast<FakeImage*>(i)->name; }
EXPORT void* il2cpp_class_from_name(void* i, const char* ns, const char* name) {
    auto img = static_cast<FakeImage*>(i);
    for (int c = 0; c < img->count; ++c)
        if (!std::strcmp(img->classes[c].ns, ns) && !std::strcmp(img->classes[c].name, name)) return &img->classes[c];
    return nullptr;
}
EXPORT void* il2cpp_class_get_method_from_name(void* k, const char* name, int argc) {
    auto klass = static_cast<FakeClass*>(k);
    for (int m = 0; m < klass->count; ++m)
        if (!std::strcmp(klass->methods[m].name, name) && (argc < 0 || klass->methods[m].argc == argc))
            return &klass->methods[m];
    return nullptr;
}
EXPORT void* il2cpp_resolve_icall(const char* name) {
    if (!std::strcmp(name, "UnityEngine.SceneManagement.Scene::GetNameInternal")) return (void*)&icall_GetNameInternal;
    if (!std::strcmp(name, "UnityEngine.SceneManagement.Scene::GetBuildIndexInternal"))
        return (void*)&icall_GetBuildIndexInternal;
    if (!std::strcmp(name, "UnityEngine.Application::set_targetFrameRate")) return (void*)&icall_set_targetFrameRate;
    return nullptr;
}
EXPORT const wchar_t* il2cpp_string_chars(void* s) { return static_cast<FakeString*>(s)->chars; }
EXPORT int32_t il2cpp_string_length(void* s) { return (int32_t)std::wcslen(static_cast<FakeString*>(s)->chars); }

// ---- driven by the fake engine / test ----

// what the real engine does on a scene load: invoke the managed method via
// its MethodInfo. calling through methodPointer means we hit the hooked code.
EXPORT void fake_engine_load_scene(int32_t handle, int32_t mode) {
    auto fn = reinterpret_cast<void (*)(int32_t, int32_t, const void*)>(g_scene_methods[0].methodPointer);
    fn(handle, mode, &g_scene_methods[0]);
}
EXPORT int fake_game_get_value() {
    auto fn = reinterpret_cast<int (*)(const void*)>(g_game_methods[0].methodPointer);
    return fn(&g_game_methods[0]);
}
EXPORT int fake_state(const char* what) {
    if (!std::strcmp(what, "init")) return g_init_called;
    if (!std::strcmp(what, "shutdown")) return g_shutdown_called;
    if (!std::strcmp(what, "scene_listener")) return g_game_scene_listener_calls;
    if (!std::strcmp(what, "target_frame_rate")) return g_target_frame_rate;
    return -999;
}
