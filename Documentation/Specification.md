# SteamFilter — product and DSP specification

**Purpose:** Real-time, CPU-conscious vocal clean-up and finishing. Windows VST3 first, Cubase primary host.

## Module chain (fixed, stereo-linked)

1. **SteamGuard** — detects transient low-frequency plosive bursts and subtracts an adaptive low-frequency component, ahead of high-pass cleanup.
2. **SteamSweep** — mouth-click attenuation, rumble high-pass, gentle low-level noise attenuation.
3. **SteamBreath** — broadband, unpitched inhalation identification and sensitive gain attenuation with adjustable naturalness and duration.
4. **SteamSibil** — high-band dynamic de-essing with frequency, width and sensitivity controls.
5. **SteamRider** — moderate, slowly smoothed vocal level riding; optional secondary input bus for mix-informed riding.
6. **WhisperLift** — gated upward compression with maximum lift, voice protection and smooth gain transitions.
7. **SteamPolish** — subtle low and high shelves plus mid-band soft nonlinearity:
   - Warm: 250 Hz / 12 kHz, light mid saturation.
   - Presence: 300 Hz / 16 kHz, low-mid saturation.
   - Air: 350 Hz / 20 kHz, very subtle higher-mid saturation.
   - At lower sample rates, shelves are clamped to a stable percentage of Nyquist.

## Visualization

- Large waveform: **actual recent input samples**, not a looping decorative PNG.
- Event overlays: yellow for breath, red for plosives, violet for click, cyan for sibilance.
- Curve: actual time-varying gain result.
- Seven small meters: **measured signal change across each module (dB)**, not input amplitude or knob position.
- Input/output are the only signal-level meters.
- DSP gauge: estimated plugin DSP execution time / available audio-block time (not full DAW system CPU).
- Every rotary dial is programmatically vector-painted and attached to an automatable parameter.

## Analyse

- Click ANALYSE while Cubase plays representative audio for 4 seconds.
- Analysis captures *statistics* only (no audio file, no network, no retained recording).
- UI presents recommendations separately, **never modifies a session without explicit Apply**.
- A no-voice/silence capture is rejected.
- Results are heuristic, not ML transcription or state-of-the-art restoration.

## Reliability and CPU policy

- DSP core has no audio-thread allocations, mutex locks, dynamic FFTs, remote calls, sample downloads or oversampling.
- Fixed 20 ms lookahead delay is reported to the host for latency compensation, and is also maintained through internal bypass.
- Sample rates 8–384 kHz are accepted by the standalone engine; 44.1 / 48 / 96 / 192 kHz are primary host targets.
- Denormal suppression, parameter gain smoothing, coefficient interpolation, output safety ceilings, finite-value checking.
- A/B banks saved within plugin state; separate user presets are XML value trees.
- No proprietary models or external cloud dependencies.

## What still requires validation

1. Compile JUCE VST3 on Windows, then pluginval at strictness 10.
2. Cubase host scanning and plugin state persistence across reloads.
3. Back-to-back heavy automation, rapid bypass, and GUI opening/closing tests.
4. Real vocal tests: male/female, whisper/falsetto/raspy, plosive blasts, breath vs unvoiced consonants, mouth clicks.
5. Artifact listening, gain-matched comparison, null/delta verification, performance on several machines.
6. A signed Windows installer is NOT generated or released until these pass.

## Honest quality notes

This is a serious engineering starting point with tested DSP behavior, not a validated commercial-grade voice-restoration system. Synthetic-source classification does not prove reliable distinction between real inhales and whispered syllables. Sophisticated mouth-click repair often requires buffered interpolation or offline processing; this real-time click attenuator has not been validated to compete with RX-grade offline repair.