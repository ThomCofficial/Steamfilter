# SteamFilter — engineering status and release gates

**Snapshot:** 2026-11-07 (unreleased). These statements describe source in this archive, not a certified VST3 product.

## Implemented in the source

- Seven processing blocks: SteamGuard, SteamSweep, SteamBreath, SteamSibil, SteamRider, WhisperLift, SteamPolish.
- Full live JUCE interface source: brushed steel/cream steampunk styling, cropped maritime header artwork, independently moving vector rotary knobs, genuine value readouts on faceplate, independently controllable and bypassable modules.
- Seven per-module before/after dB processing meters; stereo-linked input/output level meters; live waveform with multicolour event classification and distinct SteamRider/WhisperLift gain curves; measured DSP-cycle-use indicator.
- Ten-second streaming vocal analysis with suggested changes that require explicit approval. Silent/unusable input is rejected by the UI.
- Fixed 4 ms lookahead compensated through the VST3 latency API, smooth bypass and automation, real-time-safe DSP without heap allocations in the engine's process call.
- Optional audio sidechain for SteamRider, global gain match, A/B state save/restore, delta audition, factory and user presets.
- Automated C++ DSP tests and GitHub Actions workflow for future Windows builds and tests.

## Verified in this environment

- Dependency-free C++17 DSP core compiles under the available compiler.
- CTest: `SteamDSPStress` and `SteamDSPModules` pass.
- AddressSanitizer/UndefinedBehaviorSanitizer versions of both tests pass.
- Synthetic test signals produce meaningful change on each processing stage.
- CPU microbenchmark is diagnostic and machine-specific; not a guarantee about typical Cubase CPU or real-time behavior.

## NOT verified — **release blocked**

1. JUCE VST3 source has **not** been compiled against an installed JUCE SDK in this environment.
2. No `.vst3` installer or binary has been built or delivered.
3. Cubase scan, audio routing, 32/64-bit plugin validation, stereo/mono/sidechain operation and UI scaling have not been tested in the target DAW.
4. Pluginval, real recording listening trials, A/B measurements against reference restorers, stress tests on target Windows hardware, and audible artifact certification remain outstanding.
5. Breath and mouth-click classification rely on lightweight heuristic detectors. Performance on synthetic signals does **not** establish professional restoration quality on real voices.
6. The latest private `ThomCofficial/SteamFilter` repository did not appear in the current GitHub app's repository access list. Only `ThomCofficial/Steamstrip` was available. This archive has intentionally **not** been uploaded to Steamstrip.

## Build and shipping policy

- Run Windows JUCE compilation and GitHub Actions; fix all compiler issues, if any.
- Validate with pluginval at highest strictness and run stress tests in Cubase.
- Complete a documented listening test matrix of voiced/unvoiced consonants, whistles, musical breaths, different timbres, levels, room noise and automation transitions.
- Confirm performance on midrange Windows hardware without spikes before publishing any release build.
- Do not tag or label a release as complete before these gates pass.
