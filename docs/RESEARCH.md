# Community Issue Research

Sources: Steam community hub for app 2192790 (105 discussion threads and the review feed, collected 2026-10-28), the Steam store API, SteamSpy, web search, and static inspection of build 12839205.

Context: SteamSpy lists 330 positive and 389 negative reviews. The store review JSON endpoint requires a login for this 17+ title, so the review sample comes from the community hub feed. Much of the negativity concerns price and production quality, which no mod can fix. This log covers only technical defects.

**Existing modding scene: none found.** No Nexus Mods page, no PCGamingWiki page, no loader, no UnityExplorer reports. The only third-party tools are WeMod trainers. The one community "graphics guide" (a Workshop item, since removed by Steam) amounts to Unity command-line flags: `-force-d3d11 -screen-quality Beautiful -window-mode borderless`.

Labels: **confirmed** = reproduced by us; **reported** = players say so, not yet reproduced; **suspected** = plausible from code but no report; **opinion** = not a defect.

## Candidate issues

### 1. No graphics settings: frame cap, vsync, quality, window mode (reported, high confidence)
* **Reports:** thread "No graphics options or settings in 2023 is a JOKE"; review "It's 2023 and there are no graphic settings. Literally the only setting is Film Grain"; review complaining about frame rate with no way to adjust settings; thread "play in window mode"; the removed graphics guide. The pause menu classes in metadata agree: `UIFilmGrainOptions`, `UICameraOptions`, `BrightnessControllerUI`, and no graphics/display options class.
* **Reproducible:** the missing UI is verifiable statically. Frame rate behavior needs a live run.
* **Cause:** the options were never implemented on PC.
* **Can Lazarus address it:** yes. Unity's `Application.targetFrameRate`, `QualitySettings.vSyncCount`, `QualitySettings.SetQualityLevel`, `Screen.SetResolution(w, h, FullScreenMode, hz)`, `lodBias`, and `anisotropicFiltering` all exist as icalls in this `UnityPlayer.dll` (verified by name).
* **Fix:** `graphics-settings` mod, config-file driven, applied on each scene load with read-back logging.
* **Risks:** HDRP ignores some legacy settings. The game might reset values on scene change (mitigated by re-applying). Quality levels beyond the shipped ones are not possible.
* **Test:** the mod logs read-back values after applying. On a live run, confirm the frame cap with an overlay and window mode visually.

### 2. Mouse aim far too sensitive (reported, medium confidence)
* **Reports:** review "it is impossible to aim unless your DPI is at 10"; thread "Are the reviews bad because everybody is playing this with a mouse?"
* **Code:** `Lucille.CameraSystem.CameraSensitivity.SetSensitivityMultiply(multiply)` and `SensitivitySettingsData` with `_rangeMin`/`_rangeMax`. The in-game slider has a fixed range.
* **Can Lazarus address it:** likely, via a hook on `ApplySensitivity` or by scaling the multiplier. Needs a live run to calibrate.
* **Next step:** hook `SetSensitivityMultiply`, log the values the slider produces, then add a configurable scale.

### 3. No keyboard rebinding, no mouse in menus (reported)
* **Reports:** thread "Please allow the rebinding of the keyboard" (3 replies agreeing); review "There isn't even mouse support in the title screen".
* **Code:** Unity Input System with `PlayerInputsReader`. The Input System supports runtime binding overrides (`InputActionRebindingExtensions.ApplyBindingOverride`).
* **Feasibility:** plausible but needs calling managed methods with Input System objects. This is a larger task than fixes 1 and 2.

### 4. Black screen after logos (reported, one thread, self-resolved)
* The reporter fixed it by moving the install out of a path containing `C:/Steam /` (a trailing space in a folder name). **Not a Lazarus target.** Documented in troubleshooting instead.

### 5. Brightness screen: "can't apply" on new game (reported, one thread, no replies)
* `BrightnessControllerUI.Apply()` exists. With a single report and no reproduction steps, **not actionable yet**.

### 6. Progression blockers in the Governor finale (reported, two threads, no replies)
* "Freezes when the Governor gets out of the tank while Glenn and Michonne kneel" (Aug 2024); "Daryl vs Governor: final phase, can't punch or dodge, only walk" (Aug 2024, persists after reloading older saves).
* **Feasibility:** unknown. This needs a save at that point and a live debugging session. The `Lucille.BossAndrew` and `Lucille.MomentSystem` assemblies are the likely starting points (inferred from names only).

### 7. "Prison bug" thread: no usable content (the only reply is a joke). Not actionable.

### Opinions, out of scope
Slideshow cutscenes, story choices not carrying over, AI quality, graphics fidelity, price.

## Priority

1. Graphics settings (fix #1): strongest evidence, lowest risk, verifiable by read-back. **Selected as the first fix.**
2. Mouse sensitivity scaling (fix #2).
3. Key rebinding (fix #3).
4. Governor finale softlocks (fix #6): needs saves and live debugging.
