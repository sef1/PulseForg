#include "PulseForgeDSP.h"
#include <cmath>
#include <initializer_list>

namespace pulseforge
{

static inline double clamp01 (double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
static inline double midi (int note) { return 440.0 * std::pow (2.0, (note - 69) / 12.0); }

Pattern Pattern::demo()
{
    Pattern p;
    for (int i : {0, 3, 6, 8, 11, 14}) p.acidA[i].active = true;
    p.acidA[3].velocity = 2; p.acidA[3].accent = true; p.acidA[11].slide = true;
    for (int i : {2, 6, 10, 14}) p.acidB[i].active = true;
    p.acidB[6].velocity = 2; p.acidB[14].accent = true;
    for (int i : {0, 4, 8, 12}) p.drum8.kick[i] = true;
    for (int i : {4, 12}) { p.drum8.snare[i] = true; p.drum8.clap[i] = true; }
    for (int i = 0; i < 16; i += 2) p.drum8.hat[i] = true;
    for (int i : {0, 3, 7, 10, 12}) p.drum9.kick[i] = true;
    for (int i : {4, 12}) p.drum9.snare[i] = true;
    for (int i = 1; i < 16; i += 2) p.drum9.hat[i] = true;
    for (int i : {6, 14}) p.drum9.clap[i] = true;
    return p;
}

Pattern::Pattern()
{
    for (int i = 0; i < 16; ++i)
    {
        acidA[i].note = 36 + (i % 5);
        acidB[i].note = 29 + (i % 7);
    }
}

void AcidAccentModel::beginStep (bool accented, double amount)
{
    if (accented)
        sweepCharge = std::fmin (sweepCharge + 0.58 + clamp01 (amount) * 0.72, 2.35);
}

AcidAccentFrame AcidAccentModel::sample (double secondsIntoStep, bool accented, double accentAmount,
                                         double resonance, double normalDecay, double accentDecay)
{
    const double amount = clamp01 (accentAmount);
    const double accentSeconds = 0.035 + clamp01 (accentDecay) * 0.265;
    const double filterRate = accented ? 1.0 / accentSeconds
                                       : 2.0 + (1.0 - clamp01 (normalDecay)) * 15.0;
    const double filterEnvelope = std::exp (-secondsIntoStep * filterRate);

    const double attack = accented ? 1.0 - std::exp (-secondsIntoStep / 0.00155) : 0.0;
    const double amplitudeLift = accented ? amount * filterEnvelope * attack * 0.72 : 0.0;

    const double resonanceCoupling = 0.70 + clamp01 (resonance) * 0.75;
    const double accentSweep = accented ? sweepCharge * amount * resonanceCoupling : 0.0;

    sweepCharge *= std::exp (-1.0 / (sr * 0.170));
    return { filterEnvelope, accentSweep, amplitudeLift, sweepCharge };
}

namespace AcidVoiceModel
{

double slideSeconds (double slideTime) { return 0.020 + clamp01 (slideTime) * 0.480; }

double glideFrequency (double from, double to, double t, bool sliding, double slideTime)
{
    if (! sliding) return to;
    const double duration = slideSeconds (slideTime);
    if (t >= duration) return to;
    const double shaped = (1.0 - std::exp (-4.0 * t / duration)) / (1.0 - std::exp (-4.0));
    return from + (to - from) * clamp01 (shaped);
}

double oscillator (double phase, bool square)
{
    return square ? (phase < 0.5 ? 1.0 : -1.0) : phase * 2.0 - 1.0;
}

double amplitudeEnvelope (double t, double stepSeconds, bool extended,
                          double attack, double decay, double sustain, double release)
{
    if (! extended)
        return std::exp (-t * (2.0 + (1.0 - clamp01 (decay)) * 15.0));

    const double attackSeconds = 0.002 + clamp01 (attack) * 0.118;
    const double decaySeconds = 0.025 + clamp01 (decay) * 0.45;
    const double sustainLevel = 0.08 + clamp01 (sustain) * 0.90;
    const double releaseStart = stepSeconds * 0.82;
    const double body = t < attackSeconds ? t / attackSeconds
                                          : sustainLevel + (1.0 - sustainLevel) * std::exp (-(t - attackSeconds) / decaySeconds);
    return t <= releaseStart ? body : body * std::exp (-(t - releaseStart) / (0.015 + clamp01 (release) * 0.35));
}

} // namespace AcidVoiceModel

void PulseForgeEngine::prepare (double sampleRate)
{
    sr = sampleRate;
    accentA = AcidAccentModel (sampleRate);
    accentB = AcidAccentModel (sampleRate);

    // SynthEngine.kt constructor defaults: engine B and the four mixer channels.
    synth[1].tune = .55f; synth[1].cutoff = .58f; synth[1].resonance = .46f;
    synth[1].envMod = .72f; synth[1].decay = .61f; synth[1].accentAmount = .52f;

    mixer[0].pan = .42f; mixer[0].send = .30f; mixer[0].level = .78f;
    mixer[1].pan = .58f; mixer[1].send = .26f; mixer[1].level = .70f;
    mixer[2].pan = .48f; mixer[2].send = .18f; mixer[2].level = .82f;
    mixer[3].pan = .52f; mixer[3].send = .20f; mixer[3].level = .72f;

    reset();
}

void PulseForgeEngine::reset()
{
    phaseA = phaseB = 0.0;
    lpA = lpB = prevA = prevB = 0.0;
    delayL = delayR = 0.0;
    accentA.reset();
    accentB.reset();
    fromFa = fa = fromFb = fb = 0.0;
    slideA = slideB = false;
}

bool PulseForgeEngine::channelAudible (int ch) const
{
    bool anySolo = false;
    for (const auto& m : mixer) anySolo = anySolo || m.solo;
    const auto& mc = mixer[ch];
    return anySolo ? (mc.solo && ! mc.mute) : ! mc.mute;
}

void PulseForgeEngine::beginStep (int step)
{
    const auto& a = pattern->acidA[step];
    const auto& b = pattern->acidB[step];
    const int previous = (step + 15) % 16;
    const auto& previousA = pattern->acidA[previous];
    const auto& previousB = pattern->acidB[previous];
    const auto& pa = synth[0];
    const auto& pb = synth[1];

    // Kotlin: midi(a.note + ((pa.tune-.5f)*24).toInt()) - float math, truncation
    fa     = midi (a.note         + (int) ((pa.tune - .5f) * 24.0f));
    fromFa = midi (previousA.note + (int) ((pa.tune - .5f) * 24.0f));
    fb     = midi (b.note         + (int) ((pb.tune - .5f) * 24.0f));
    fromFb = midi (previousB.note + (int) ((pb.tune - .5f) * 24.0f));

    slideA = previousA.active && previousA.slide && a.active;
    slideB = previousB.active && previousB.slide && b.active;

    accentA.beginStep (a.active && a.accent, pa.accentAmount);
    accentB.beginStep (b.active && b.accent, pb.accentAmount);
}

double PulseForgeEngine::nextRandom()
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng >> 1) * (1.0 / 2147483648.0) - 1.0;
}

void PulseForgeEngine::renderSample (int step, double t, long long i, double stepSeconds,
                                     float& outL, float& outR)
{
    const auto& a = pattern->acidA[step];
    const auto& b = pattern->acidB[step];
    const auto& pa = synth[0];
    const auto& pb = synth[1];
    const auto& d8 = pattern->drum8;
    const auto& d9 = pattern->drum9;

    double acidA = 0.0, acidB = 0.0, drums8 = 0.0, drums9 = 0.0;

    if (a.active)
    {
        const double currentFa = AcidVoiceModel::glideFrequency (fromFa, fa, t, slideA, pa.slideTime);
        phaseA = std::fmod (phaseA + currentFa / sr, 1.0);
        const double raw = AcidVoiceModel::oscillator (phaseA, pa.squareWave);
        const auto accentFrame = accentA.sample (t, a.accent, pa.accentAmount, pa.resonance, pa.decay, pa.accentDecay);
        const double ampEnv = AcidVoiceModel::amplitudeEnvelope (t, stepSeconds, pa.extendedEnvelope,
                                                                 pa.attack, pa.decay, pa.sustain, pa.release);
        const double sweep = 1.0 + pa.envMod * accentFrame.filterEnvelope * 1.8 + accentFrame.filterSweep * 0.34;
        const double coeff = std::fmin (std::fmax (0.012 + (double) (pa.cutoff * pa.cutoff) * 0.34 * sweep, 0.01), 0.48);
        lpA += coeff * (raw - lpA + pa.resonance * (lpA - prevA) * 1.7);
        prevA = lpA;
        const double velocity = a.velocity == 2 ? 1.28 : 1.0;
        acidA = lpA * ampEnv * 0.31 * velocity * (1.0 + accentFrame.amplitudeLift);
    }

    if (b.active)
    {
        const double currentFb = AcidVoiceModel::glideFrequency (fromFb, fb, t, slideB, pb.slideTime);
        phaseB = std::fmod (phaseB + currentFb / sr, 1.0);
        const double raw = AcidVoiceModel::oscillator (phaseB, pb.squareWave);
        const auto accentFrame = accentB.sample (t, b.accent, pb.accentAmount, pb.resonance, pb.decay, pb.accentDecay);
        const double ampEnv = AcidVoiceModel::amplitudeEnvelope (t, stepSeconds, pb.extendedEnvelope,
                                                                 pb.attack, pb.decay, pb.sustain, pb.release);
        const double sweep = 1.0 + pb.envMod * accentFrame.filterEnvelope * 1.8 + accentFrame.filterSweep * 0.34;
        const double coeff = std::fmin (std::fmax (0.012 + (double) (pb.cutoff * pb.cutoff) * 0.34 * sweep, 0.01), 0.48);
        lpB += coeff * (raw - lpB + pb.resonance * (lpB - prevB) * 1.7);
        prevB = lpB;
        const double velocity = b.velocity == 2 ? 1.28 : 1.0;
        acidB = lpB * ampEnv * 0.28 * velocity * (1.0 + accentFrame.amplitudeLift);
    }

    const double twoPi = 2.0 * 3.14159265358979323846;

    if (d8.kick[step])  drums8 += std::sin (twoPi * (48.0 + 105.0 * std::exp (-t * 30.0)) * t) * std::exp (-t * 13.0) * 0.58;
    if (d8.snare[step]) drums8 += (nextRandom() * 0.7 + std::sin (twoPi * 185.0 * t) * 0.3) * std::exp (-t * 22.0) * 0.24;
    if (d8.hat[step])   drums8 += nextRandom() * std::sin (twoPi * 6200.0 * t) * std::exp (-t * 65.0) * 0.10;
    if (d8.clap[step] && (i % 337 < 85)) drums8 += nextRandom() * std::exp (-t * 17.0) * 0.13;

    if (d9.kick[step])  drums9 += (std::sin (twoPi * (56.0 + 155.0 * std::exp (-t * 38.0)) * t) * std::exp (-t * 19.0)
                                   + (i < 80 ? nextRandom() * 0.2 : 0.0)) * 0.48;
    if (d9.snare[step]) drums9 += (nextRandom() * 0.8 + std::sin (twoPi * 205.0 * t) * 0.2) * std::exp (-t * 27.0) * 0.22;
    if (d9.hat[step])   drums9 += nextRandom() * std::sin (twoPi * 7800.0 * t) * std::exp (-t * 78.0) * 0.09;
    if (d9.clap[step] && (i % 271 < 65)) drums9 += nextRandom() * std::exp (-t * 22.0) * 0.12;

    const double voices[4] = { acidA, acidB, drums8, drums9 };
    double left = 0.0, right = 0.0, sendL = 0.0, sendR = 0.0;
    for (int ch = 0; ch < 4; ++ch)
    {
        if (! channelAudible (ch)) continue;
        const auto& mc = mixer[ch];
        const double signal = voices[ch] * mc.level;
        const double l = signal * (1.0 - mc.pan * 0.78);
        const double r = signal * (0.22 + mc.pan * 0.78);
        left += l; right += r;
        sendL += l * mc.send; sendR += r * mc.send;
    }

    delayL = delayL * 0.91 + sendR * 0.09;
    delayR = delayR * 0.91 + sendL * 0.09;
    left += delayL * 0.22; right += delayR * 0.22;

    left  = std::tanh (left  * (1.0 + drive * 3.0)) * 0.78;
    right = std::tanh (right * (1.0 + drive * 3.0)) * 0.78;

    outL = (float) (left < -1.0 ? -1.0 : (left > 1.0 ? 1.0 : left));
    outR = (float) (right < -1.0 ? -1.0 : (right > 1.0 ? 1.0 : right));
}

} // namespace pulseforge
