// Interop tests for StrudelBridgeCore against the real Kotlin StrudelBridge:
// export strings must be byte-identical; import results (bpm, applied,
// skipped, pattern) must match Kotlin's on the same inputs.

#include "../Source/StrudelBridgeCore.h"
#include "../Source/PatternBankCodec.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace pulseforge;

static std::string readFile (const char* path)
{
    std::ifstream in (path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::vector<std::string> lines (const std::string& s)
{
    std::vector<std::string> out;
    std::istringstream in (s);
    std::string line;
    while (std::getline (in, line)) out.push_back (line);
    return out;
}

// Parses the Kotlin canonical import output and compares with our result.
static void compareImport (const std::string& kotlinOut, const std::string& input,
                           const char* label)
{
    auto r = StrudelBridgeCore::importCode (input);

    std::string kBpm, kApplied, kSkipped, kBanks;
    for (const auto& line : lines (kotlinOut))
    {
        if (line.rfind ("bpm=", 0) == 0) kBpm = line.substr (4);
        else if (line.rfind ("applied=", 0) == 0) kApplied = line.substr (8);
        else if (line.rfind ("skipped=", 0) == 0) kSkipped = line.substr (8);
        else if (line.rfind ("banks=", 0) == 0) kBanks = line.substr (6);
    }

    const std::string myBpm = r.hasBpm ? StrudelBridgeCore::trimFloat (r.bpm) : "null";
    if (kBpm != "null" && r.hasBpm)
        assert (std::fabs (std::strtof (kBpm.c_str(), nullptr) - r.bpm) < 0.001f);
    else
        assert (kBpm == myBpm);

    const std::string myApplied = StrudelBridgeCore::join (r.applied, ",");
    const std::string mySkipped = StrudelBridgeCore::join (r.skipped, ",");
    if (kApplied != myApplied || kSkipped != mySkipped)
    {
        std::fprintf (stderr, "[%s] applied/skipped mismatch\nKotlin: %s | %s\nC++:    %s | %s\n",
                      label, kApplied.c_str(), kSkipped.c_str(), myApplied.c_str(), mySkipped.c_str());
        assert (false);
    }

    if (kBanks == "null")
    {
        assert (! r.hasPattern);
    }
    else
    {
        assert (r.hasPattern);
        Pattern kotlinBanks[8];
        assert (PatternBankCodec::decodeBanks (kBanks, kotlinBanks));
        Pattern mine[8];
        mine[0] = r.pattern;
        // Kotlin PatternBankStore(it) puts the imported pattern in slot 0.
        const auto encK = PatternBankCodec::encodePattern (kotlinBanks[0]);
        const auto encM = PatternBankCodec::encodePattern (mine[0]);
        if (encK != encM)
        {
            std::fprintf (stderr, "[%s] pattern mismatch\nKotlin: %s\nC++:    %s\n",
                          label, encK.c_str(), encM.c_str());
            assert (false);
        }
    }
    std::printf ("[%s] import matches Kotlin\n", label);
}

int main (int argc, char** argv)
{
    // argv: <kotlin_export> <kotlin_export_waves> <kotlin_url> <kotlin_import_on_our_export> <kotlin_import_on_tricky> <tricky_input> <kotlin_import_on_url>
    assert (argc == 8);
    const Pattern demo = Pattern::demo();

    // 1) Export byte-identity with the real Kotlin codec.
    const std::string myExport = StrudelBridgeCore::exportCode (demo, 126.0f, false, false);
    const std::string kExport = readFile (argv[1]);
    if (myExport != kExport)
    {
        std::fprintf (stderr, "export mismatch\nKotlin:\n%s\nC++:\n%s\n", kExport.c_str(), myExport.c_str());
        return 1;
    }
    const std::string myWaves = StrudelBridgeCore::exportCode (demo, 126.0f, true, true);
    if (myWaves != readFile (argv[2])) { std::fprintf (stderr, "exportWaves mismatch\n"); return 1; }
    std::printf ("export byte-identical (%zu bytes)\n", myExport.size());

    // 2) Import of our export: same results as Kotlin importing it.
    compareImport (readFile (argv[4]), myExport, "roundtrip");

    // 3) Tricky hand-made input: names, mixed lanes, >16 tokens, extra machines.
    compareImport (readFile (argv[5]), readFile (argv[6]), "tricky");

    // 4) strudel.cc long URL: Kotlin produced it; we must import it identically.
    compareImport (readFile (argv[7]), readFile (argv[3]), "url");

    std::printf ("StrudelBridgeTests OK\n");
    return 0;
}
