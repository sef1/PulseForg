#pragma once

#include <cstdint>

/**
 * PulseForge DSP core - M2.
 *
 * 1:1 double-precision port of the Android engine (SynthEngine.kt,
 * AcidVoiceModel.kt, AcidAccentModel.kt, Pattern.kt). No JUCE or platform
 * dependencies so the Android app, the plugin, and the offline render
 * harnesses all execute identical math.
 *
 * Sequencing stays outside: the host feeds step boundaries (beginStep) and
 * asks for samples (renderSample) with t = seconds into the current step.
 */

namespace pulseforge
{

struct AcidStep
{
    bool active = false;
    int  note = 36;
    bool accent = false;
    int  velocity = 1;
    bool slide = false;
};

struct DrumPattern
{
    bool kick[16] = {};
    bool snare[16] = {};
    bool hat[16] = {};
    bool clap[16] = {};
};

struct Pattern
{
    Pattern(); // applies Kotlin note defaults: acidA[i]=36+(i%5), acidB[i]=29+(i%7)

    AcidStep acidA[16];
    AcidStep acidB[16];
    DrumPattern drum8;
    DrumPattern drum9;

    static Pattern demo(); // 1:1 port of Pattern.demo() in Pattern.kt
};

struct SynthParams
{
    float tune = .5f;
    float cutoff = .58f;
    float resonance = .42f;
    float envMod = .62f;
    float decay = .54f;
    float accentAmount = .68f;
    bool  squareWave = false;
    bool  extendedEnvelope = false;
    float attack = .08f;
    float sustain = .62f;
    float release = .18f;
    float slideTime = .30f;
    float accentDecay = .35f;
};

struct MixerChannel
{
    float pan = .5f;
    float send = .25f;
    float level = .75f;
    bool  mute = false;
    bool  solo = false;
};

struct AcidAccentFrame
{
    double filterEnvelope;
    double filterSweep;
    double amplitudeLift;
    double retainedCharge;
};

/** 1:1 port of AcidAccentModel.kt. */
class AcidAccentModel
{
public:
    explicit AcidAccentModel (double sampleRate) : sr (sampleRate) {}

    void beginStep (bool accented, double amount);
    AcidAccentFrame sample (double secondsIntoStep, bool accented, double accentAmount,
                            double resonance, double normalDecay, double accentDecay);
    void reset() { sweepCharge = 0.0; }

private:
    double sr;
    double sweepCharge = 0.0;
};

/** 1:1 port of AcidVoiceModel.kt. */
namespace AcidVoiceModel
{
    double slideSeconds (double slideTime);
    double glideFrequency (double from, double to, double t, bool sliding, double slideTime);
    double oscillator (double phase, bool square);
    double amplitudeEnvelope (double t, double stepSeconds, bool extended,
                              double attack, double decay, double sustain, double release);
}

/**
 * The four-voice engine: ACID A, ACID B, DRUM 8, DRUM 9 through the
 * four-channel mixer, send delay and tanh drive. Mirrors SynthEngine.kt's
 * render loop, including its state retention quirks (accent sweep charge
 * only decays on active steps; the slide flag lives on the step slid FROM).
 */
class PulseForgeEngine
{
public:
    void prepare (double sampleRate);
    void reset();              // phases, filters, accent models, delay lines
    void setPattern (const Pattern& p) { pattern = &p; }

    bool channelAudible (int ch) const;

    /** Call once at each step boundary (sample-accurate) before rendering it. */
    void beginStep (int step);

    /** t = seconds into the step; i = sample index into the step (clap gates). */
    void renderSample (int step, double t, long long i, double stepSeconds,
                       float& outL, float& outR);

    SynthParams  synth[2];
    MixerChannel mixer[4];
    float drive = .25f;

private:
    double nextRandom(); // xorshift32, [-1, 1) - replaces kotlin.random.Random

    const Pattern* pattern = nullptr;
    double sr = 44100.0;

    double phaseA = 0.0, phaseB = 0.0;
    double lpA = 0.0, lpB = 0.0, prevA = 0.0, prevB = 0.0;
    double delayL = 0.0, delayR = 0.0;

    AcidAccentModel accentA { 44100.0 }, accentB { 44100.0 };

    // Per-step precomputed state (beginStep)
    double fromFa = 0.0, fa = 0.0, fromFb = 0.0, fb = 0.0;
    bool slideA = false, slideB = false;

    uint32_t rng = 0x9E3779B9u;
};

} // namespace pulseforge
