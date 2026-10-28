# Stage 1: Technical Feasibility

Assessment date: 2026-10-28. Everything marked **verified** was checked directly against the installed game files. Everything marked **inferred** follows from that evidence but has not been observed at runtime.

## Target build (verified)

| Item | Value | How it was checked |
|---|---|---|
| Steam App ID | 2192790 | `steam_appid.txt` |
| Steam build ID | 12839205 | local install metadata |
| Unity version | 2021.3.17f1 (file version 2021.3.17.4096273) | `UnityPlayer.dll` version resource, `globalgamemanagers` |
| Scripting backend | IL2CPP | `GameAssembly.dll` present, no `Managed/` folder |
| IL2CPP metadata | v29, unencrypted, 18,148,708 bytes | header magic `0xFAB11BAF`, version field 29 |
| Render pipeline | HDRP | `Unity.RenderPipelines.HighDefinition.Runtime.dll` in `ScriptingAssemblies.json` |
| Input | Unity Input System (new) | `Unity.InputSystem.dll` |
| Architecture | x64 | PE header of `UnityPlayer.dll` |
| Developer code | ~150 `Lucille.*` / `Flux.*` assemblies plus `Assembly-CSharp` | metadata image table |

Install layout (no `Managed/` folder, which is what IL2CPP builds look like):

```
The Walking Dead Destinies.exe     653 KB launcher stub, imports only UnityPlayer.dll + KERNEL32
UnityPlayer.dll                    engine
GameAssembly.dll                   all C# compiled to native by IL2CPP
baselib.dll
The Walking Dead Destinies_Data/
  il2cpp_data/Metadata/global-metadata.dat
  Plugins/x86_64/                  fmod, steam_api64, burst, resonance audio, vhacd
```

## Consequence of IL2CPP

There is no Mono runtime in the process. A normal .NET DLL cannot be loaded into the game, and Harmony-style IL patching does not apply. Game logic exists only as native x64 code in `GameAssembly.dll`. The reflection data in `global-metadata.dat` remains available through the exported `il2cpp_*` C API (235 exports, verified with `dumpbin /exports`).

BepInEx handles this by embedding a full CoreCLR runtime and generating proxy assemblies (Il2CppInterop). That is a lot of machinery and is exactly the dependency the project brief rules out. We don't need it for the fixes in scope.

## Selected architecture

A small **native C++ loader** whose mods are native DLLs with a C ABI.

1. **Bootstrap: `version.dll` proxy (verified viable).** `UnityPlayer.dll` statically imports `VERSION.dll`. Windows resolves that from the game directory before System32 because `version.dll` is not a KnownDLL. Our proxy forwards all 17 real exports to `%SystemRoot%\System32\version.dll`. **The stock game ships no `version.dll`, so installation adds one file and replaces nothing.**
2. **Runtime hand-off: intercept `il2cpp_init` (verified viable, inferred to work at runtime).** `UnityPlayer.dll` imports `GetProcAddress` from KERNEL32 and resolves the `il2cpp_*` API from `GameAssembly.dll` dynamically. During `DllMain` we patch that one IAT slot in `UnityPlayer.dll`. When Unity asks for `il2cpp_init`, it gets our wrapper, which calls the real function and then initializes the loader on the main thread with a live IL2CPP domain. No threads are created and no work happens under the loader lock.
3. **Engine-ready event: hook `SceneManager.Internal_SceneLoaded(scene, mode)` (verified present).** The engine calls this managed method natively on every scene load, so it survives IL2CPP stripping. We resolve it through `il2cpp_class_get_method_from_name` and inline-hook its native body. Mods get `on_scene_loaded`, which is the first point where calling engine APIs is safe. It costs nothing per frame.
4. **Hooking: MinHook (BSD-2-Clause), vendored.** This is the only third-party code, and it is a library, not a mod loader.
5. **Engine calls: `il2cpp_resolve_icall`.** Unity's internal calls such as `UnityEngine.QualitySettings::set_vSyncCount` are plain C functions that can be called directly. All the icalls used by the first fix were verified by name in `UnityPlayer.dll`.
6. **Mods**: `Lazarus/mods/<id>/mod.ini` plus a DLL that exports `lazarus_mod_init`. Mods are **off until explicitly enabled** in `Lazarus/lazarus.ini`, because a native DLL is arbitrary code.

## Toolchain (verified installed)

MSVC 14.44 (VS 2022 Community + Build Tools), CMake, Git 2.52, gh 2.95, Python 3.14, Windows PowerShell 5.1 with Pester 3.4. No disassembler is needed for the first fix. `tools/il2cpp_meta.py` (ours) is sufficient for name discovery.

## Major risks

* **Unity might not resolve `il2cpp_init` through the patched IAT** (for example, a cached pointer or a different resolution path). Mitigation: the loader logs at every bootstrap step. If `il2cpp_init` is never seen, the log says so and the game runs unmodified. The fallback is hooking `LoadLibraryW("GameAssembly.dll")` and inline-hooking the export.
* **The game may overwrite engine settings** after we set them. Mitigation: re-apply on every scene load, read back with getters, and log any drift.
* **HDRP ignores some legacy `QualitySettings`** (shadow distance and anti-aliasing are controlled by HDRP assets). The first fix logs readback values instead of assuming they took effect.
* **Game updates** change `GameAssembly.dll`. The loader fingerprints the build and blocks mods that declare a specific build when the fingerprint doesn't match.
* **Native mods can crash the game.** SEH guards around mod entry points, a boot-lock file that auto-disables a mod that crashed during init, and safe mode.

## Can be built and tested without running the game

Config and manifest parsing, mod discovery, dependency ordering, compatibility checks, crash-lock recovery, the proxy's export forwarding, and **the full bootstrap chain**. That last one is covered by an end-to-end test harness with a fake `UnityPlayer.dll` that imports `version.dll`, `LoadLibrary`s a fake `GameAssembly.dll`, and calls `il2cpp_init` through `GetProcAddress`, the same pattern the real engine uses. The installer can also be tested against a fake game directory.

## Requires the actual game

Confirming that the real `UnityPlayer.dll` resolves `il2cpp_init` through the patched slot, that the `Internal_SceneLoaded` hook fires, and that the graphics fix's values stick under HDRP.
