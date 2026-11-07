# SteamFilter — Cubase acceptance checklist (for the downloadable engineering TEST build)

**A successful GitHub Actions VST3 compile does not prove that the GUI or detectors sound right in Cubase.** Do not label this release-ready until these tests are independently confirmed on Windows 11 / Cubase 13.

## Graphical interface

1. Confirm the whole approved cream/steel/brass maritime interface fills the plugin and the lettering is engraved on the faceplate, not scrambled across controls.
2. Turn **every** knob slowly and quickly; visible pointer must rotate while the associated Cubase automatable parameter changes. Double-click for JUCE reset to defaults.
3. Verify every power toggle, mode button, preset selector, A/B, analysis, bypass, delta, SteamRider sidechain and WhisperLift voice protection. Parameter values must save and reload with the project.
4. The small seven module meters must remain at zero when that module is bypassed, then show measured stage attenuation (green) or boost (amber). They are not input meters.
5. The large waveform must animate from actual source audio; breath=yellow, plosive=red, click=violet, sibilance=blue, Rider=green trace and WhisperLift=amber trace. It must not display an example waveform while stopped.
6. Check output tubes (real measured output peak), DSP load gauge (actual processing time), resizable UI, and labels at 100%, 125%, and 150% Windows UI scale.

## Seven processing modules (always loudness match while comparing)

1. SteamSweep: vocal with mouth clicks, low-frequency rumble and audible room noise; isolate each feature and ensure artifacts do not damage vowels.
2. SteamBreath: inhale before a phrase; confirm breath attenuation only when detected, preserve natural character, then verify the next sung consonant is not attenuated.
3. SteamGuard: spoken plosives P/B; ensure low-frequency burst is reduced while bass warmth on vowels remains.
4. SteamSibil: S/SH/CH phrases; confirm split-band reduction where indicated and no permanent top-end dulling.
5. SteamRider: whisper-to-belt changes, with optional external sidechain; vary target, range and speed; verify maximum range.
6. WhisperLift: low-level spoken/sung details; ensure the gain comes up on voiced content, never boosts gaps aggressively with Voice Protect on.
7. SteamPolish: Warm/Presence/Air and Amount 0–100%; check all modes at 44.1, 48, 96 and 192 kHz. No aliasing spikes or harshness from mild character saturation.

## Regression and safety

- Analyze while Cubase plays a representative vocal: a four-second capture should finish and show suggestions; **suggestions require manual approval**, and should not change audio until applied.
- On a mono channel and stereo channel, repeatedly toggle module on/off, delta, A/B, global bypass and global dry/wet while automation is running; no sharp clicks, exploding output or host crashes.
- At 44.1 and 48 kHz, the plugin reports constant 20 ms latency to Cubase for stable PDC; the user-adjustable lookahead controls the detector lead from 0–20 ms without changing PDC.
- Load 10–20 instances, log real Cubase CPU percentage, and monitor dropouts at 64/128/256/512 samples; the decorative gauge shows the *individual plugin processing-time ratio*, **not** total Cubase CPU.
- Compare the audio against clean unprocessed source and third-party vocal cleanup tools. A commercial-grade restoration claim requires real-vocal listening tests on varied singers, microphones, languages and rooms.

The current test suite uses generated signals; it cannot replace these listening tests.