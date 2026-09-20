# PulseForge VST3 (Cubase, Windows)

VST3 instrument port of PulseForge, the Android acid groovebox. Targets Cubase
on Windows first. Same GPL-3.0-only license as the app.

## Status: M1 - transport skeleton

Silent VST3 instrument that loads in Cubase and shows a transport debug
readout: host tempo, play state, PPQ position, and the live 16-step position
derived from the host timeline. No sound yet by design.

Milestones:

- **M1 (this)**: JUCE 9 VST3 shell, host-transport step counter, validation
  clean (pluginval + Steinberg validator in CI).
- **M2**: DSP port from the Android `SynthEngine`/`AcidVoiceModel`/
  `AcidAccentModel` Kotlin sources, sample-accurate host-synced stepping,
  sound matched against Android reference renders.
- **M3**: full rack UI, automatable parameters, project JSON in the DAW
  state chunk, Strudel bridge.
- **M4**: beta on a real Cubase machine.

## Build (Windows)

Requirements: Visual Studio 2022 (MSVC), CMake 3.22+. JUCE 9.0.2 is fetched
automatically.

```
cmake -S plugin -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target PulseForge_VST3
```

Output: `build/PulseForge_artefacts/Release/VST3/PulseForge.vst3`

Every push to `plugin/**` also builds on GitHub Actions (`windows-latest`),
runs pluginval (strictness 8, headless) and Steinberg's validator, and
uploads the `.vst3` as a workflow artifact.

## Install in Cubase

Copy `PulseForge.vst3` (the whole folder) to:

```
C:\Program Files\Common Files\VST3\
```

Then in Cubase: Studio > VST Plug-in Manager > rescan, and insert PulseForge
on an Instrument track. For M1, press play and watch the step LEDs track the
host position; change tempo and confirm the steps stay locked.

## Tests

Transport mapping unit tests (no JUCE needed):

```
g++ -std=c++17 -o /tmp/TransportSyncTests plugin/Tests/TransportSyncTests.cpp && /tmp/TransportSyncTests
```

## Licensing

PulseForge is GPL-3.0-only (see repo root LICENSE). This plugin combines:

- PulseForge code (GPL-3.0-only)
- JUCE 9 modules, used under the AGPLv3 (https://juce.com/legal/)
- Steinberg VST3 SDK interfaces via JUCE; the VST3 SDK is MIT-licensed
  since SDK 3.8 (2025-10)

GPLv3 section 13 permits combining GPLv3 code with AGPLv3 code; the combined
work is distributed under GPLv3 with source, same as the Android app.
