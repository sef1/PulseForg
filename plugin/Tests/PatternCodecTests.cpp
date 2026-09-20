// Round-trip and golden-vector tests for PatternBankCodec.
// Golden vector: encode output of the real Kotlin PatternBankStore codec,
// produced by Tests/reference/BankCodecGolden.kt with kotlinc.

#include "../Source/PatternBankCodec.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using pulseforge::Pattern;
using pulseforge::PatternBankCodec::decodeBanks;
using pulseforge::PatternBankCodec::decodePattern;
using pulseforge::PatternBankCodec::encodeBanks;
using pulseforge::PatternBankCodec::encodePattern;

static bool equal (const Pattern& a, const Pattern& b)
{
    for (int i = 0; i < 16; ++i)
    {
        const auto& x = a.acidA[i]; const auto& y = b.acidA[i];
        if (x.active != y.active || x.note != y.note || x.accent != y.accent
            || x.velocity != y.velocity || x.slide != y.slide) return false;
        const auto& p = a.acidB[i]; const auto& q = b.acidB[i];
        if (p.active != q.active || p.note != q.note || p.accent != q.accent
            || p.velocity != q.velocity || p.slide != q.slide) return false;
        if (a.drum8.kick[i] != b.drum8.kick[i] || a.drum8.snare[i] != b.drum8.snare[i]
            || a.drum8.hat[i] != b.drum8.hat[i] || a.drum8.clap[i] != b.drum8.clap[i]) return false;
        if (a.drum9.kick[i] != b.drum9.kick[i] || a.drum9.snare[i] != b.drum9.snare[i]
            || a.drum9.hat[i] != b.drum9.hat[i] || a.drum9.clap[i] != b.drum9.clap[i]) return false;
    }
    return true;
}

static std::string readTrimmed (const char* path)
{
    std::ifstream in (path);
    std::stringstream ss;
    ss << in.rdbuf();
    auto s = ss.str();
    while (! s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

int main (int argc, char** argv)
{
    // 1) Round trip: demo in bank 0, factory defaults in banks 1-7
    //    (exactly what the plugin ships and what PatternBankStore() holds).
    Pattern banks[8];
    banks[0] = Pattern::demo();
    const std::string encoded = encodeBanks (banks);

    Pattern back[8];
    assert (decodeBanks (encoded, back));
    for (int i = 0; i < 8; ++i)
        assert (equal (banks[i], back[i]));

    // 2) Single-pattern round trip.
    {
        Pattern p = Pattern::demo();
        p.acidA[5].active = true; p.acidA[5].note = 51; p.acidA[5].slide = true;
        p.acidB[9].accent = true; p.acidB[9].velocity = 2;
        p.drum9.clap[3] = true;
        Pattern q;
        assert (decodePattern (encodePattern (p), q));
        assert (equal (p, q));
    }

    // 3) Corrupt inputs rejected, banks untouched.
    {
        Pattern sentinel[8];
        sentinel[0] = Pattern::demo();
        Pattern before[8];
        for (int i = 0; i < 8; ++i) before[i] = sentinel[i];
        assert (! decodeBanks ("garbage", sentinel));
        assert (! decodeBanks (encoded + "|extra", sentinel));
        assert (! decodePattern ("1,36,0,1,0;", sentinel[0])); // 1 step, not 16
        for (int i = 0; i < 8; ++i) assert (equal (before[i], sentinel[i]));
    }

    // 4) Golden vector: must be byte-identical to the real Kotlin codec's
    //    output for the same content.
    if (argc > 1)
    {
        const std::string kotlin = readTrimmed (argv[1]);
        if (kotlin != encoded)
        {
            std::fprintf (stderr, "MISMATCH vs Kotlin codec\nC++:    %s\nKotlin: %s\n",
                          encoded.c_str(), kotlin.c_str());
            return 1;
        }
        Pattern fromKotlin[8];
        assert (decodeBanks (kotlin, fromKotlin));
        for (int i = 0; i < 8; ++i)
            assert (equal (banks[i], fromKotlin[i]));
        std::printf ("golden vector matches Kotlin codec (%zu bytes)\n", kotlin.size());
    }

    std::printf ("PatternCodecTests OK\n");
    return 0;
}
