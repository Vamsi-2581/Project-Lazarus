# Architecture

Background and the reasoning behind these choices are in [FEASIBILITY.md](FEASIBILITY.md). This page covers how the pieces fit together.

## Code layout

```
include/lazarus/lazarus_api.h   the only header mods see (C ABI)
src/core/                       pure logic, no game dependency, unit tested
    ini          INI parser
    log          file logger with session rotation
    manifest     mod.ini parsing + validation
    config       lazarus.ini
    plan         discovery, enable list, compat, deps, ordering
    state        crash lock, quarantine, build fingerprint
src/proxy/                      becomes version.dll
    version_forward.asm + .def   17 export stubs -> System32 version.dll
    iat.cpp                      import table patcher
    dllmain.cpp                  boot: forwarding + GetProcAddress interception
    runtime.cpp                  everything after il2cpp_init: API, scene hook, mod lifecycle
mods/graphics-settings/         first fix
installer/                      Install-Lazarus.ps1 + .bat wrappers
tests/                          unit, Pester installer tests, e2e harness
tools/il2cpp_meta.py            metadata reader for finding hook targets
third_party/minhook/            vendored, unmodified
```

## Boot sequence

```
The Walking Dead Destinies.exe
  └─ loads UnityPlayer.dll
       └─ static import VERSION.dll  ──► Lazarus version.dll (game folder wins over System32)
            DllMain: LoadLibrary(System32\version.dll), fill 17 forward pointers
                     patch UnityPlayer's IAT slot for KERNEL32!GetProcAddress
                     write bootstrap.log
  UnityPlayer: LoadLibrary("GameAssembly.dll")
               GetProcAddress(ga, "il2cpp_init")      ──► returns lz_il2cpp_init
               GetProcAddress(ga, "il2cpp_shutdown")  ──► returns lz_il2cpp_shutdown
               lz_il2cpp_init(): real il2cpp_init, then runtime start (main thread)
```

Runtime start, in order: open `lazarus.log`, read `lazarus.ini`, log the environment, fingerprint `GameAssembly.dll`, resolve the il2cpp API, start MinHook, hook `SceneManager.Internal_SceneLoaded`, consume a stale boot lock (quarantine), check for safe mode, discover mods, plan, then load each mod in order.

Loading one mod: write `boot.lock` with the mod id, `LoadLibraryEx`, find `lazarus_mod_init`, call it under SEH, delete `boot.lock`. On failure, its hooks and callbacks are removed.

Scene load: the original `Internal_SceneLoaded` runs first (the game's own listeners), then each mod callback runs under SEH. A callback that faults is disabled; the game continues.

Shutdown (`il2cpp_shutdown`): each mod's `lazarus_mod_shutdown` runs in reverse order, all hooks are removed, MinHook uninitializes, and the log closes. Then the real `il2cpp_shutdown` runs.

## Failure policy

The loader never takes the game down on purpose. Every failure turns into "this mod or feature is off" plus a log line:

| failure | result |
|---|---|
| proxy can't patch UnityPlayer | bootstrap.log says so, game runs stock |
| il2cpp export missing | no mods load, game runs stock |
| loader itself throws during start | caught, logged, game continues |
| mod init returns non-zero or throws | that mod's hooks are removed, it's skipped |
| process dies inside mod init | boot.lock survives, mod is quarantined next launch |
| scene callback faults | that callback is disabled |
| bad config value | default used, warning logged |

## Performance

There is no per-frame work. The loader does its work once at startup and once per scene load. The `GameAssembly.dll` fingerprint is cached by size and mtime, so the 59 MB hash only runs when the file changes.
