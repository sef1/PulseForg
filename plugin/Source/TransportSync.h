#pragma once

#include <cmath>

namespace pulseforge
{

/**
 * Maps host transport position (PPQ, quarter notes) onto the PulseForge
 * 16-step sequencer grid (16th notes in 4/4).
 *
 * Pure math, no JUCE or platform dependencies, so it can be unit-tested
 * standalone and reused identically on Android and in the plugin.
 *
 * M1 uses block-granular lookups (one step index per processBlock).
 * Sample-accurate step triggering lands in M2 with the DSP port.
 */
struct TransportStep
{
    static constexpr int    kStepsPerBar     = 16;
    static constexpr double kStepsPerQuarter = 4.0;

    // One step at 120 BPM is ~4.5e-5 quarter-notes long, so an epsilon of
    // 1e-9 steps only snaps across float-representation noise, never audio.
    static constexpr double kEpsilon = 1e-9;

    /** Step index 0-15 for a host PPQ position. Safe for negative PPQ (preroll). */
    static int stepIndexForPpq (double ppq)
    {
        const auto s = static_cast<long long> (std::floor (ppq * kStepsPerQuarter + kEpsilon));
        long long m = s % kStepsPerBar;
        if (m < 0)
            m += kStepsPerBar;
        return static_cast<int> (m);
    }

    /** Position inside the current step, 0.0 (inclusive) to 1.0 (exclusive). */
    static double phaseInStep (double ppq)
    {
        const double steps = ppq * kStepsPerQuarter;
        return steps - std::floor (steps);
    }
};

} // namespace pulseforge
