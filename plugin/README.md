# PulseForge VST3 (Cubase, Windows)

VST3 instrument port of PulseForge, the Android acid groovebox. Targets Cubase
on Windows first. Same GPL-3.0-only license as the app.

## Status: M2 - full DSP, demo pattern

The complete PulseForge engine runs in Cubase on the host transport: acid
engines A/B, drum 8/9 machines, four-channel mixer, send delay and drive,
sequenced sample-accurately from the host PPQ position. M2 plays the
built-in demo pattern; parameters, banks and project state land in M3.

The DSP is a 1:1 double-precision port of the Android engine. Verified
against a reference renderer compiled from the actual Kotlin sources
(`plugin/Tests/reference/`): acid engines bit-exact over 8 bars at
126 BPM, drum energies statistically identical (PRNGs differ by design).

Milestones:

- **M1**: JUCE 9 VST3 shell, host-transport step counter. Field-tested in
  Cubase on Windows.
- **M2 (this)**: DSP port, sample-accurate host-synced stepping, validated
  against Android reference renders.
- **M3**: full rack UI, automatable parameters, project JSON in the DAW
  state chunk, pattern banks, Strudel bridge.
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

No JUCE needed for any of these:

```
g++ -std=c++17 -o /tmp/TransportSyncTests plugin/Tests/TransportSyncTests.cpp && /tmp/TransportSyncTests
g++ -std=c++17 -o /tmp/SequencerTests plugin/Tests/SequencerTests.cpp plugin/Source/DSP/PulseForgeDSP.cpp && /tmp/SequencerTests
g++ -std=c++17 -O2 -o /tmp/RenderHarness plugin/Tests/RenderHarness.cpp plugin/Source/DSP/PulseForgeDSP.cpp && /tmp/RenderHarness /tmp/out.wav
```

Reference render from the real Kotlin sources (needs JDK + kotlinc):

```
kotlinc app/src/main/java/com/sefi/pulseforge/{AcidVoiceModel,AcidAccentModel,Pattern}.kt \
  plugin/Tests/reference/ReferenceRender.kt -include-runtime -d /tmp/ref.jar
java -jar /tmp/ref.jar /tmp/ref.wav [nodrums]
```

## Licensing

PulseForge is GPL-3.0-only (see repo root LICENSE). This plugin combines:

- PulseForge code (GPL-3.0-only)
- JUCE 9 modules, used under the AGPLv3 (https://juce.com/legal/)
- Steinberg VST3 SDK interfaces via JUCE; the VST3 SDK is MIT-licensed
  since SDK 3.8 (2025-10)

GPLv3 section 13 permits combining GPLv3 code with AGPLv3 code; the combined
work is distributed under GPLv3 with source, same as the Android app.
