# Testing

```
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

configures, builds, and runs all three suites through ctest. CI runs the same thing on `windows-2022` for every push.

## Suites

| suite | what it covers |
|---|---|
| `core_unit_tests` | INI parsing, logging and rotation, manifest validation (including path traversal), config defaults and bad values, discovery, load planning (enable list, dependencies, cycles, API version, build pinning, quarantine), crash lock, fingerprinting, graphics-settings config parsing |
| `installer_tests` (Pester 3/4) | missing-file detection, Mono/untested-build warnings, install, uninstall back to a byte-identical folder, foreign `version.dll` backup/restore, settings preserved on reinstall, refusal to delete a replaced `version.dll`, interrupted install, Steam library discovery |
| `e2e_bootstrap` | the real `version.dll` proxy inside a fake Unity process (see below), 14 scenarios |

### End-to-end harness

`tests/e2e` builds a folder that mirrors the game's structure. `fake_game.exe` imports `UnityPlayer.dll`, which imports `VERSION.dll` and resolves `il2cpp_init` from `GameAssembly.dll` through its own `GetProcAddress` import, the same shape as the real engine (confirmed with `dumpbin /imports` against the shipped `UnityPlayer.dll`). The real proxy is copied in as `version.dll`. Each scenario is a fresh process:

happy path (forwarding, interception, hooks, scene dispatch, icalls, clean shutdown), mod not enabled, safe mode, loader disabled, init returns an error, init throws, an access violation in a scene callback, a hard process kill during init, quarantine on the next launch plus un-quarantine, a corrupted manifest beside a good mod, a missing `lazarus.ini`, and three graphics-settings scenarios (applies config and survives a quality-preset reset and the game resetting values between scenes; an empty config changes nothing; bad values are rejected while good ones apply).

## What this does and does not prove

**Verified locally:** everything above, plus static checks against the shipped game files: the import tables, the absence of delay-loading, the il2cpp exports, the metadata version, the icall names, and that the installer finds and validates the real install on a custom drive path.

**Not verified yet (needs a live run of the real game):**

1. That the real `UnityPlayer.dll` resolves `il2cpp_init` through the patched IAT slot. Strong static evidence, not observed.
2. That the `Internal_SceneLoaded` hook fires in the real engine.
3. That Unity's icalls are registered and that graphics settings stick under HDRP.
4. That `il2cpp_shutdown` is called on quit (if not, mods just don't get a shutdown callback).

## Live verification procedure

Game: The Walking Dead: Destinies, Steam build 12839205 (fingerprint `1b579b75b1486c0e`), Unity 2021.3.17f1.

1. Install with `Install.bat`.
2. In `Lazarus\mods\graphics-settings\config.ini` set `target_fps = 45` and `vsync = 0`. 45 is an unusual value, so it's easy to confirm.
3. Start the game, reach the main menu, open an FPS overlay (Steam overlay → in-game FPS counter).
4. Expected: about 45 FPS. `lazarus.log` contains `scene load hook installed`, `scene loaded: '<name>'` lines, `[mod:graphics-settings] frame rate cap set to 45`, and `startup complete: 1 mod(s) active`.
5. Quit normally. Expected: `shutdown complete` at the end of the log.
6. Hold SHIFT while starting. Expected: `safe mode: SHIFT held during startup`, normal frame rate.
7. Run `Uninstall.bat`. Expected: game folder back to stock.

Please attach `lazarus.log` and `bootstrap.log` when reporting results.
