#include "../Source/DSP/PulseForgeDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

using pulseforge::AcidResonantFilter;

static double tailEnergy (double resonance, double sampleRate)
{
    AcidResonantFilter f; f.prepare (sampleRate);
    double e = 0.0, peak = 0.0;
    for (int i = 0; i < (int) (sampleRate * .25); ++i)
    {
        const double input = i < 16 ? 1.0 : 0.0;
        const double y = f.process (input, .57, 1.0, resonance);
        assert (std::isfinite (y));
        peak = std::max (peak, std::abs (y));
        if (i > (int) (sampleRate * .025)) e += y * y;
    }
    assert (peak <= 1.000001);
    return e;
}

int main()
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const double low = tailEnergy (.05, sr);
        const double high = tailEnergy (1.0, sr);
        std::printf ("sr %.0f low tail %.6g high tail %.6g ratio %.1f\n", sr, low, high, high / low);
        assert (high > low * 20.0);
    }
    std::puts ("Resonance tests passed");
}
