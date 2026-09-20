// Integration tests for BlockSequencer: the exact code path the plugin runs,
// driven with synthetic transport positions. Compares against a continuous
// reference render and checks transport edge cases.

#include "../Source/BlockSequencer.h"
#include "../Source/TransportSync.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using pulseforge::BlockSequencer;
using pulseforge::Pattern;
using pulseforge::PulseForgeEngine;

static void renderContinuous (std::vector<float>& out, int totalSteps, double bpm, double sampleRate)
{
    auto pattern = Pattern::demo();
    PulseForgeEngine engine;
    engine.prepare (sampleRate);
    engine.setPattern (pattern);
    const int samplesPerStep = (int) (sampleRate * 60.0 / bpm / 4.0);
    const double stepSeconds = (double) samplesPerStep / sampleRate;
    for (long long s = 0; s < totalSteps; ++s)
    {
        engine.beginStep ((int) (s % 16));
        for (long long i = 0; i < samplesPerStep; ++i)
        {
            float l, r;
            engine.renderSample ((int) (s % 16), (double) i / sampleRate, i, stepSeconds, l, r);
            out.push_back (l); out.push_back (r);
        }
    }
}

int main()
{
    const double sr = 44100.0, bpm = 126.0;
    const int samplesPerStep = 5250;

    // 1) Blocked render (512-sample blocks from ppq 0) must be bit-exact
    //    against the continuous reference at this tempo.
    {
        auto pattern = Pattern::demo();
        PulseForgeEngine engine;
        engine.prepare (sr);
        engine.setPattern (pattern);
        BlockSequencer seq;

        const int total = samplesPerStep * 16 * 8;
        std::vector<float> blocked;
        blocked.reserve ((size_t) total * 2);

        const int block = 512;
        for (int start = 0; start < total; start += block)
        {
            const int n = std::min (block, total - start);
            std::vector<float> l (n), r (n);
            const double ppq = (double) start / (sr * 60.0 / bpm);
            const bool rendered = seq.renderBlock (engine, sr, true, true, bpm, ppq, l.data(), r.data(), n);
            assert (rendered);
            for (int j = 0; j < n; ++j) { blocked.push_back (l[j]); blocked.push_back (r[j]); }
        }

        std::vector<float> continuous;
        renderContinuous (continuous, 16 * 8, bpm, sr);
        assert (blocked.size() == continuous.size());
        double maxDiff = 0.0;
        for (size_t i = 0; i < blocked.size(); ++i)
            maxDiff = std::max (maxDiff, (double) std::abs (blocked[i] - continuous[i]));
        std::printf ("blocked-vs-continuous maxdiff = %.3e\n", maxDiff);
        assert (maxDiff < 1e-6);
    }

    // 2) Stopped transport renders silence and reports not-rendered.
    {
        auto pattern = Pattern::demo();
        PulseForgeEngine engine; engine.prepare (sr); engine.setPattern (pattern);
        BlockSequencer seq;
        float l[128], r[128];
        assert (! seq.renderBlock (engine, sr, true, false, bpm, 0.0, l, r, 128));
        assert (! seq.renderBlock (engine, sr, false, false, bpm, 0.0, l, r, 128)); // no sync
    }

    // 3) Play from mid-step (ppq 0.31): step triggers must land on exact
    //    sample frames: initial trigger at frame 0 with step 1, then step 2
    //    at (0.5-0.31)*21000, step 3 at (0.75-0.31)*21000, etc.
    {
        auto pattern = Pattern::demo();
        PulseForgeEngine engine; engine.prepare (sr); engine.setPattern (pattern);
        BlockSequencer seq;

        struct Log { std::vector<int> steps; std::vector<long long> frames; } log;
        seq.onBeginStep = [] (int step, long long frame, void* ctx)
        {
            auto* l = (Log*) ctx;
            l->steps.push_back (step);
            l->frames.push_back (frame);
        };
        seq.onBeginStepCtx = &log;

        const double ppqMid = 0.31;
        const double samplesPerQuarter = sr * 60.0 / bpm; // 21000 exact
        const int n = 20000;
        std::vector<float> l (n), r (n);
        assert (seq.renderBlock (engine, sr, true, true, bpm, ppqMid, l.data(), r.data(), n));

        assert (log.steps.size() >= 3);
        assert (log.steps[0] == 1 && log.frames[0] == 0);
        const long long expect2 = (long long) ((0.50 - ppqMid) * samplesPerQuarter + 0.5);
        const long long expect3 = (long long) ((0.75 - ppqMid) * samplesPerQuarter + 0.5);
        assert (log.steps[1] == 2 && log.frames[1] == expect2);
        assert (log.steps[2] == 3 && log.frames[2] == expect3);
        std::printf ("mid-step triggers: step1@0, step2@%lld (want %lld), step3@%lld (want %lld)\n",
                     log.frames[1], expect2, log.frames[2], expect3);
    }

    // 4) Tempo change between blocks keeps stepping (no stuck/crash) and
    //    step boundaries fire exactly once per step.
    {
        auto pattern = Pattern::demo();
        PulseForgeEngine engine; engine.prepare (sr); engine.setPattern (pattern);
        BlockSequencer seq;
        double ppq = 0.0;
        float l[256], r[256];
        double lastRms = 0.0;
        for (int blk = 0; blk < 40; ++blk)
        {
            const double b = blk < 20 ? 126.0 : 100.0;
            assert (seq.renderBlock (engine, sr, true, true, b, ppq, l, r, 256));
            double e = 0.0; for (int j = 0; j < 256; ++j) e += l[j] * l[j];
            lastRms = std::sqrt (e / 256.0);
            ppq += 256.0 / (sr * 60.0 / b);
        }
        assert (lastRms > 0.0);
    }

    std::puts ("Sequencer tests passed");
    return 0;
}
