# ThomC SteamFilter

**SteamFilter** is a steampunk-inspired, real-time vocal refinement VST3 project: seven modular processors, animated vector-drawn rotary controls, actual gain-change metering, event-coloured waveform, manual-review analysis suggestions, A/B snapshots, and an efficient standalone C++ DSP engine.

**Development status: unreleased engineering source, NOT a finished/certified VST3.** No downloadable production binary is included. A professional release needs Windows JUCE compilation, host stress testing and real-vocal listening validation. The project deliberately never publishes an installer just because compilation succeeded.

### Modules

SteamGuard (plosives) → SteamSweep (clicks/rumble/noise) → SteamBreath (inhalations) → SteamSibil (de-essing) → SteamRider (leveling) → WhisperLift (upward compression) → SteamPolish (air/body/character).

### Test the DSP without JUCE

```shell
cmake -S . -B build/test -DBUILD_TESTING=ON -DSTEAMFILTER_BUILD_PLUGIN=OFF
cmake --build build/test --config Release
ctest --test-dir build/test -C Release --output-on-failure
```

### Build the experimental VST3 on Windows

```shell
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -DSTEAMFILTER_BUILD_PLUGIN=ON
cmake --build build/win --config Release --target SteamFilter_VST3
ctest --test-dir build/win -C Release --output-on-failure
```

CMake automatically retrieves JUCE **9.0.4** when network is available. Alternatively, place a JUCE checkout in `JUCE/` locally (ignored by Git). Windows 10/11, Visual Studio 2022, CMake ≥3.22 are the intended build environment. Copying an unvalidated VST3 into Cubase is not recommended.

### Visual assets and functionality

Only `Assets/MaritimeHeader.jpg` is a static decorative asset from the approved nautical concept; all knobs, buttons, meters, waveform and processing indicators are live JUCE components. No animated PNG/GIF sprite hacks; knobs physically rotate by procedural painting when values change. Slider labels and numerical readouts live on the faceplates, not on knob surfaces.

### Licensing

Copyright © 2026 ThomC. All rights reserved. Not licensed for third-party redistribution. JUCE is available under its own dual licensing terms; review commercial licensing **before distributing** proprietary VST3 releases. No external ML libraries or commercial audio models are bundled.

See [Documentation/Specification.md](Documentation/Specification.md) for design decisions and [Documentation/EngineeringStatus.md](Documentation/EngineeringStatus.md) for exact verified/unverified release gates.
