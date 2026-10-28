// Mod used by the end-to-end tests. Exercises every API entry point and can
// be told to misbehave via its config.ini:
//
//   [test]
//   behavior = normal | fail_init | throw_init | exit_in_init | crash_in_scene
//   target_fps = 90

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "lazarus/lazarus_api.h"

namespace {

const LazarusApi* g_api = nullptr;
LazarusMod* g_self = nullptr;
int (*g_orig_get_value)(const void*) = nullptr;
void (*g_set_fps)(int) = nullptr;
std::string g_events;
char g_behavior[32] = "normal";
int g_scene_count = 0;

void event(const char* what) {
    std::string path = std::string(g_api->mod_dir(g_self)) + "\\events.txt";
    FILE* f = std::fopen(path.c_str(), "a");
    if (f) {
        std::fprintf(f, "%s\n", what);
        std::fclose(f);
    }
}

int hk_get_value(const void* mi) {
    int orig = g_orig_get_value(mi);
    return orig == 1 ? 42 : -1;
}

void on_scene(void*, const char* name, int idx, int mode) {
    ++g_scene_count;
    char buf[128];
    std::snprintf(buf, sizeof buf, "scene:%s:%d:%d", name, idx, mode);
    event(buf);
    if (!std::strcmp(g_behavior, "crash_in_scene") && g_scene_count == 1) {
        volatile int* p = nullptr;
        *p = 1;  // deliberate access violation, loader should disable this callback
    }
    if (g_set_fps) g_set_fps((int)g_api->config_get_int(g_self, "test", "target_fps", 60));
}

}  // namespace

extern "C" __declspec(dllexport) int lazarus_mod_init(LazarusMod* self, const LazarusApi* api) {
    g_api = api;
    g_self = self;
    api->config_get_string(self, "test", "behavior", g_behavior, sizeof g_behavior);
    api->log(self, LAZARUS_LOG_INFO, "test mod init, behavior=%s, api v%u, build %s", g_behavior, api->api_version,
             api->game_build);
    event("init");

    if (api->struct_size < sizeof(LazarusApi)) return 10;

    void* mi = api->find_method("Lucille.Fake.dll", "Lucille.Fake", "FakeGame", "GetValue", 0);
    if (!mi) return 11;
    if (api->find_method("Lucille.Fake.dll", "Lucille.Fake", "NoSuchClass", "GetValue", 0)) return 12;
    if (api->hook_create(self, api->method_pointer(mi), (void*)&hk_get_value, (void**)&g_orig_get_value) != LAZARUS_OK)
        return 13;
    g_set_fps = reinterpret_cast<void (*)(int)>(api->resolve_icall("UnityEngine.Application::set_targetFrameRate"));
    if (!g_set_fps) return 14;
    if (api->resolve_icall("UnityEngine.Nope::nothing")) return 15;
    if (api->on_scene_loaded(self, &on_scene, nullptr) != LAZARUS_OK) return 16;

    // malformed value -> default + loader warning
    if (api->config_get_bool(self, "test", "not_a_bool", 1) != 1) return 17;

    if (!std::strcmp(g_behavior, "fail_init")) return 5;
    if (!std::strcmp(g_behavior, "throw_init")) RaiseException(0xE0000001, 0, 0, nullptr);
    if (!std::strcmp(g_behavior, "exit_in_init")) TerminateProcess(GetCurrentProcess(), 3);
    return 0;
}

extern "C" __declspec(dllexport) void lazarus_mod_shutdown(LazarusMod*) { event("shutdown"); }
