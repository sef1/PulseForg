// Offline render harness for the PulseForge DSP core.
// Usage: RenderHarness <out.wav> [nodrums]
// Renders 8 bars of Pattern::demo() at 126 BPM / 44100 Hz (the Android
// engine's defaults; 5250 samples per step - exact 16ths, no truncation).

#include "../Source/DSP/PulseForgeDSP.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static void writeWavFloat (const char* path, const std::vector<float>& interleaved, int sampleRate, int channels)
{
    FILE* f = std::fopen (path, "wb");
    if (! f) { std::fprintf (stderr, "cannot open %s\n", path); std::exit (1); }
    const uint32_t dataSize = (uint32_t) (interleaved.size() * sizeof (float));
    const uint32_t fmtSize = 16;
    const uint16_t audioFormat = 3; // IEEE float
    auto wr = [f] (const void* p, size_t n) { std::fwrite (p, 1, n, f); };
    uint32_t u32; uint16_t u16;
    wr ("RIFF", 4); u32 = 36 + dataSize; wr (&u32, 4); wr ("WAVE", 4);
    wr ("fmt ", 4); wr (&fmtSize, 4); wr (&audioFormat, 2);
    u16 = (uint16_t) channels; wr (&u16, 2);
    u32 = (uint32_t) sampleRate; wr (&u32, 4);
    u32 = (uint32_t) (sampleRate * channels * 4); wr (&u32, 4);
    u16 = (uint16_t) (channels * 4); wr (&u16, 2);
    u16 = 32; wr (&u16, 2);
    wr ("data", 4); wr (&dataSize, 4);
    std::fwrite (interleaved.data(), sizeof (float), interleaved.size(), f);
    std::fclose (f);
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::fprintf (stderr, "usage: %s <out.wav> [nodrums]\n", argv[0]); return 1; }
    const bool noDrums = argc > 2 && std::strcmp (argv[2], "nodrums") == 0;

    const double sampleRate = 44100.0;
    const double bpm = 126.0;
    const int samplesPerStep = (int) (sampleRate * 60.0 / bpm / 4.0); // 5250, exact
    const double stepSeconds = (double) samplesPerStep / sampleRate;
    const int totalSteps = 16 * 8;

    auto pattern = pulseforge::Pattern::demo();
    if (noDrums)
    {
        pattern.drum8 = pulseforge::DrumPattern();
        pattern.drum9 = pulseforge::DrumPattern();
    }

    pulseforge::PulseForgeEngine engine;
    engine.prepare (sampleRate);
    engine.setPattern (pattern);

    std::vector<float> pcm;
    pcm.reserve ((size_t) totalSteps * samplesPerStep * 2);

    for (long long stepAbs = 0; stepAbs < totalSteps; ++stepAbs)
    {
        const int step = (int) (stepAbs % 16);
        engine.beginStep (step);
        for (long long i = 0; i < samplesPerStep; ++i)
        {
            float l, r;
            engine.renderSample (step, (double) i / sampleRate, i, stepSeconds, l, r);
            pcm.push_back (l);
            pcm.push_back (r);
        }
    }

    writeWavFloat (argv[1], pcm, (int) sampleRate, 2);
    std::printf ("rendered %zu frames to %s%s\n", pcm.size() / 2, argv[1], noDrums ? " (nodrums)" : "");
    return 0;
}
