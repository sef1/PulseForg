# PulseForge

PulseForge is a clean-room Android acid groovebox with two monophonic synth voices, two original synthesized drum sections, a four-channel mixer, pattern and project storage, and a Strudel text bridge.

## 0.7.2 features

- Two independent 16-step acid-inspired synth voices with saw/square selection
- Per-step note, octave, velocity, accent, and slide editing
- Separate slide-time and accent-decay controls for each synth
- Two original synthesized drum sections, each with kick, snare, hat, and clap
- Four-channel mixer with level, pan, send, mute, and solo controls
- Eight pattern banks plus project save/load
- Strudel code export/import and one-tap opening of generated code at strudel.cc
- Portrait phone interface for Android 8.0 and later

## Strudel bridge

PulseForge generates and parses a limited subset of Strudel pattern text. It can copy generated code or open that code at [strudel.cc](https://strudel.cc/). It does not embed or distribute the Strudel engine.

The conversion is intentionally limited. Notes, basic drum events, tempo, and waveform hints can be transferred. Accents, slides, velocity, filter settings, and other PulseForge-specific behavior are not converted.

Strudel is a separate AGPL-3.0 project. PulseForge is not affiliated with or endorsed by Strudel.

## Clean-room scope

PulseForge uses original branding, interface code, synthesis code, and generated drum sounds. It contains no copied proprietary code, samples, names, interface assets, circuitry, or other assets from Roland ReBirth or Roland instruments. Its acid-style behavior is an original implementation informed by public descriptions, not copied code or circuitry. PulseForge is not affiliated with or endorsed by Roland.

## Build

Requirements:

- JDK 17
- Android SDK 35
- Android Build Tools supported by Android Gradle Plugin 8.6.1

Set `sdk.dir` in `local.properties`, then run:

```sh
./gradlew test assembleDebug
```

The debug APK is written to `app/build/outputs/apk/debug/app-debug.apk`.

## License

PulseForge is licensed under the [GNU General Public License v3.0 only](LICENSE) (`GPL-3.0-only`). If you distribute a modified version, you must provide the corresponding source under GPLv3.
