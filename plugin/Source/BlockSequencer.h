#pragma once

#include <cmath>
#include <limits>

#include "DSP/PulseForgeDSP.h"

namespace pulseforge
{

/**
 * Sample-accurate host-transport sequencing for one audio block.
 * JUCE-free so the exact code the plugin runs is unit-testable offline.
 *
 * Contract: ppq is the host position (quarter notes) at the first sample of
 * the block; tempo is assumed constant inside the block (standard practice).
 * beginStep fires on the first sample of each new 16th-note step.
 * On the playing -> stopped edge the engine is reset and the block is silent,
 * matching the Android engine's stop/start behaviour.
 */
struct BlockSequencer
{
    bool wasPlaying = false;
    long long lastStepFloor = 0;

    // Test instrumentation: invoked with (stepIndex, frameInBlock) at every
    // step trigger. Null in production.
    void (*onBeginStep) (int step, long long frame, void* ctx) = nullptr;
    void* onBeginStepCtx = nullptr;

    /** Returns true if audio was rendered this block. */
    bool renderBlock (PulseForgeEngine& engine, double sampleRate,
                      bool transportValid, bool playing,
                      double bpm, double ppqAtBlockStart,
                      float* outL, float* outR, int numSamples)
    {
        if (! transportValid || ! playing || bpm <= 0.0)
        {
            if (wasPlaying)
            {
                engine.reset();
                wasPlaying = false;
            }
            return false;
        }

        if (! wasPlaying)
        {
            engine.reset();
            lastStepFloor = std::numeric_limits<long long>::min();
            wasPlaying = true;
        }

        const double samplesPerQuarter = sampleRate * 60.0 / bpm;
        const double samplesPerStep = samplesPerQuarter / 4.0;
        const double stepSeconds = 60.0 / bpm / 4.0;

        for (int j = 0; j < numSamples; ++j)
        {
            const double ppqJ = ppqAtBlockStart + (double) j / samplesPerQuarter;
            const double stepPos = ppqJ * 4.0;
            const long long stepFloor = (long long) std::floor (stepPos);

            long long m = stepFloor % 16;
            if (m < 0) m += 16;
            const int stepIdx = (int) m;

            if (stepFloor != lastStepFloor)
            {
                engine.beginStep (stepIdx);
                lastStepFloor = stepFloor;
                if (onBeginStep != nullptr)
                    onBeginStep (stepIdx, (long long) j, onBeginStepCtx);
            }

            const double frac = stepPos - (double) stepFloor;
            const double t = frac * stepSeconds;
            const long long iInStep = (long long) (frac * samplesPerStep + 0.5);

            float l = 0.0f, r = 0.0f;
            engine.renderSample (stepIdx, t, iInStep, stepSeconds, l, r);
            outL[j] = l;
            outR[j] = r;
        }
        return true;
    }
};

} // namespace pulseforge
