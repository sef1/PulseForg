#pragma once

#include <string>
#include <vector>
#include <cstdlib>

#include "DSP/PulseForgeDSP.h"

namespace pulseforge
{

/**
 * 1:1 port of PatternBankStore.kt's wire codec. JUCE-free so the exact code
 * the plugin runs is unit-testable offline against the real Kotlin codec.
 *
 * Pattern string: 16 acidA steps, '/', 16 acidB steps, '/', 64 drum8 chars,
 * '/', 64 drum9 chars.
 * Step:   "active,note,accent,velocity,slide;" (bools as 0/1).
 * Drum:   4 chars per step: kick,snare,hat,clap.
 * Banks:  8 pattern strings joined by '|'.
 */
namespace PatternBankCodec
{
    inline void appendStep (std::string& s, const AcidStep& st)
    {
        s += st.active ? '1' : '0'; s += ',';
        s += std::to_string (st.note); s += ',';
        s += st.accent ? '1' : '0'; s += ',';
        s += std::to_string (st.velocity); s += ',';
        s += st.slide ? '1' : '0'; s += ';';
    }

    inline std::string encodePattern (const Pattern& p)
    {
        std::string s;
        s.reserve (4 * 16 * 8 + 128);
        for (const auto& st : p.acidA) appendStep (s, st);
        s += '/';
        for (const auto& st : p.acidB) appendStep (s, st);
        s += '/';
        auto drum = [&s] (const DrumPattern& d)
        {
            for (int i = 0; i < 16; ++i)
            {
                s += d.kick[i]  ? '1' : '0';
                s += d.snare[i] ? '1' : '0';
                s += d.hat[i]   ? '1' : '0';
                s += d.clap[i]  ? '1' : '0';
            }
        };
        drum (p.drum8); s += '/';
        drum (p.drum9);
        return s;
    }

    inline std::vector<std::string> split (const std::string& s, char delim)
    {
        std::vector<std::string> out;
        std::string cur;
        for (const char c : s)
        {
            if (c == delim) { out.push_back (cur); cur.clear(); }
            else cur += c;
        }
        out.push_back (cur);
        return out;
    }

    inline bool parseInt (const std::string& s, int& out)
    {
        if (s.empty()) return false;
        char* end = nullptr;
        const long v = std::strtol (s.c_str(), &end, 10);
        if (end == s.c_str() || *end != '\0') return false;
        out = (int) v;
        return true;
    }

    inline bool parseSteps (const std::string& text, AcidStep* to)
    {
        std::vector<std::string> items;
        for (auto& p : split (text, ';'))
            if (! p.empty()) items.push_back (p);
        if (items.size() != 16) return false;
        for (int i = 0; i < 16; ++i)
        {
            const auto f = split (items[i], ',');
            if (f.size() != 5) return false;
            to[i].active = f[0] == "1";
            if (! parseInt (f[1], to[i].note)) return false;
            to[i].accent = f[2] == "1";
            if (! parseInt (f[3], to[i].velocity)) return false;
            to[i].slide = f[4] == "1";
        }
        return true;
    }

    inline bool parseDrum (const std::string& text, DrumPattern& d)
    {
        if (text.size() != 64) return false;
        for (int i = 0; i < 16; ++i)
        {
            d.kick[i]  = text[i * 4]     == '1';
            d.snare[i] = text[i * 4 + 1] == '1';
            d.hat[i]   = text[i * 4 + 2] == '1';
            d.clap[i]  = text[i * 4 + 3] == '1';
        }
        return true;
    }

    inline bool decodePattern (const std::string& s, Pattern& p)
    {
        const auto groups = split (s, '/');
        if (groups.size() != 4) return false;
        return parseSteps (groups[0], p.acidA)
            && parseSteps (groups[1], p.acidB)
            && parseDrum (groups[2], p.drum8)
            && parseDrum (groups[3], p.drum9);
    }

    inline std::string encodeBanks (const Pattern* banks) // exactly 8
    {
        std::string s;
        for (int i = 0; i < 8; ++i)
        {
            if (i > 0) s += '|';
            s += encodePattern (banks[i]);
        }
        return s;
    }

    /** All-or-nothing, matching Kotlin restore(): banks untouched on failure. */
    inline bool decodeBanks (const std::string& s, Pattern* banks)
    {
        const auto values = split (s, '|');
        if (values.size() != 8) return false;
        Pattern tmp[8];
        for (int i = 0; i < 8; ++i)
            if (! decodePattern (values[i], tmp[i])) return false;
        for (int i = 0; i < 8; ++i) banks[i] = tmp[i];
        return true;
    }
}

} // namespace pulseforge
