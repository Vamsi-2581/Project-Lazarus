# Contributing

## Ground rules

* **No game files in the repo.** Not binaries, not `global-metadata.dat`, not metadata dumps, not extracted assets. `.gitignore` blocks the obvious ones; keep your own dumps in `local/`.
* **No DRM or Steam authentication bypasses.** Lazarus doesn't touch Steam and must keep it that way.
* **No other mod loader as a dependency.** Using libraries is fine (MinHook is), but the loader is ours.
* **Claims need evidence.** Don't mark something "fixed" in docs or issues without saying how it was verified. "Tested against the fake engine" and "tested in game" are different statements; say which one applies.

## Workflow

1. Build and test: `powershell -ExecutionPolicy Bypass -File scripts/build.ps1`
2. Keep commits small and focused, in conventional style: `feat(loader): ...`, `fix(installer): ...`, `docs: ...`, `test(...)`.
3. Every change to `src/core` gets a unit test. Every change to proxy/runtime behavior gets an e2e scenario in `tests/e2e/e2e_runner.cpp` if it can be expressed there.
4. CI must be green.

## Writing a fix

1. Find the evidence: a Steam thread, a review, a reproducible case. Add it to [docs/RESEARCH.md](docs/RESEARCH.md).
2. Find the code: `python tools/il2cpp_meta.py <global-metadata.dat> --grep <Name>`.
3. Start from [examples/hello-mod](examples/hello-mod). Read [docs/MOD_API.md](docs/MOD_API.md).
4. If you hook game methods (not just Unity icalls), set `game_builds` in `mod.ini` to the fingerprint you tested on.
5. Document how you verified it, ideally with the exact log lines.

## Code style

Match the surrounding code: C++17, 4-space indent, `snake_case` functions, comments that explain *why*. The core library stays free of Windows game specifics where possible, so it remains unit-testable.

## Reporting results from the real game

The most valuable contribution right now is running the [live verification procedure](docs/TESTING.md#live-verification-procedure) and posting the logs in an issue.
