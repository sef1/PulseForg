#pragma once

#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cctype>

#include "DSP/PulseForgeDSP.h"

namespace pulseforge
{

/**
 * 1:1 port of StrudelBridge.kt (JUCE-free). Converts PulseForge pattern data
 * to and from Strudel pattern text; the strudel.cc long URL is
 * 'https://strudel.cc/#' + urlencode(base64(code)). Strudel is AGPL-3.0 and
 * is never embedded - this is a text interchange format only.
 *
 * Unsupported-note sentinel: valid notes clamp to 12..96, so 0 marks an
 * unsupported token (Kotlin null) and -1 a rest (Kotlin "~").
 */
namespace StrudelBridgeCore
{
    inline std::string join (const std::vector<std::string>& v, const char* sep)
    {
        std::string s;
        for (size_t i = 0; i < v.size(); ++i) { if (i) s += sep; s += v[i]; }
        return s;
    }

    inline std::vector<std::string> splitWs (const std::string& s)
    {
        std::vector<std::string> out;
        std::istringstream in (s);
        std::string tok;
        while (in >> tok) out.push_back (tok);
        return out;
    }

    inline std::string trim (const std::string& s)
    {
        const auto b = s.find_first_not_of (" \t\r\n");
        const auto e = s.find_last_not_of (" \t\r\n");
        return b == std::string::npos ? "" : s.substr (b, e - b + 1);
    }

    // Kotlin trim(): integral floats print without decimals, otherwise the
    // shortest round-trip form (approximated with %.6g, plenty for 40-300 bpm).
    inline std::string trimFloat (float v)
    {
        if (v == (float) (long) v) return std::to_string ((long) v);
        char buf[32];
        std::snprintf (buf, sizeof buf, "%.6g", (double) v);
        return buf;
    }

    inline std::string acidTokens (const AcidStep* steps)
    {
        std::vector<std::string> v;
        for (int i = 0; i < 16; ++i)
            v.push_back (steps[i].active ? std::to_string (steps[i].note) : "~");
        return join (v, " ");
    }

    inline std::string laneTokens (const bool* lane, const char* name)
    {
        std::vector<std::string> v;
        for (int i = 0; i < 16; ++i) v.push_back (lane[i] ? name : "~");
        return join (v, " ");
    }

    inline std::string exportCode (const Pattern& p, float bpm, bool waveA, bool waveB)
    {
        static const char* const drumNames[4] = { "bd", "sd", "hh", "cp" };
        std::string s;
        s += "// PulseForge export - accents, slides, velocity and filter settings have no Strudel equivalent here\n";
        s += "setcps(" + trimFloat (bpm / 240.0f) + ")\n";
        s += "stack(\n";
        s += "  note(\"" + acidTokens (p.acidA) + "\")";
        s += ".s(\"" + std::string (waveA ? "square" : "sawtooth") + "\"),\n";
        s += "  note(\"" + acidTokens (p.acidB) + "\")";
        s += ".s(\"" + std::string (waveB ? "square" : "sawtooth") + "\")";
        const DrumPattern* drums[2] = { &p.drum8, &p.drum9 };
        const char* labels[2] = { "DRUM 8", "DRUM 9" };
        for (int d = 0; d < 2; ++d)
        {
            const bool* lanes[4] = { drums[d]->kick, drums[d]->snare, drums[d]->hat, drums[d]->clap };
            for (int lane = 0; lane < 4; ++lane)
            {
                s += ",\n  // " + std::string (labels[d]) + " " + drumNames[lane] + "\n";
                s += "  s(\"" + laneTokens (lanes[lane], drumNames[lane]) + "\")";
            }
        }
        s += "\n)\n";
        return s;
    }

    inline std::string urlDecode (const std::string& s)
    {
        std::string out;
        auto hex = [] (char c) -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '%' && i + 2 < s.size())
            {
                const int hi = hex (s[i + 1]), lo = hex (s[i + 2]);
                if (hi >= 0 && lo >= 0) { out += (char) (hi * 16 + lo); i += 2; continue; }
            }
            out += s[i] == '+' ? ' ' : s[i];
        }
        return out;
    }

    inline std::string base64Decode (const std::string& in)
    {
        static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<int> map (256, -1);
        for (int i = 0; i < 64; ++i) map[(unsigned char) chars[i]] = i;
        std::string out;
        int val = 0, bits = -8;
        for (const unsigned char c : in)
        {
            if (c == '=') break;
            if (map[c] < 0) continue;
            val = (val << 6) + map[c];
            bits += 6;
            if (bits >= 0)
            {
                out += (char) ((val >> bits) & 0xFF);
                bits -= 8;
            }
        }
        return out;
    }

    struct ImportResult
    {
        bool hasPattern = false;
        Pattern pattern;
        bool hasBpm = false;
        float bpm = 0.0f;
        std::vector<std::string> applied, skipped;
    };

    inline bool parseFloatStrict (const std::string& s, float& out)
    {
        if (s.empty()) return false;
        char* end = nullptr;
        out = std::strtof (s.c_str(), &end);
        return end != s.c_str() && *end == '\0';
    }

    // "a/b/c" left-associative division; a zero divisor after the first is invalid.
    inline bool parseNumber (const std::string& expr, float& out)
    {
        bool have = false;
        float value = 0.0f;
        size_t start = 0;
        while (true)
        {
            const size_t slash = expr.find ('/', start);
            const std::string part = trim (expr.substr (start, slash == std::string::npos ? slash : slash - start));
            float n;
            if (! parseFloatStrict (part, n)) return false;
            if (n == 0.0f && have) return false;
            value = have ? value / n : n;
            have = true;
            if (slash == std::string::npos) break;
            start = slash + 1;
        }
        out = value;
        return true;
    }

    constexpr int REST = -1;

    inline int parseNoteToken (const std::string& token)
    {
        if (token == "~") return REST;
        if (token.find_first_of ("*[]<>,!?@{}|\\") != std::string::npos) return 0;
        char* end = nullptr;
        const long asInt = std::strtol (token.c_str(), &end, 10);
        if (end != token.c_str() && *end == '\0')
            return (asInt >= 0 && asInt <= 127) ? (int) std::min<long> (96, std::max<long> (12, asInt)) : 0;
        static const std::regex noteRe ("^([a-gA-G])([#sb]?)(-?[0-9])?$");
        std::smatch m;
        if (! std::regex_match (token, m, noteRe)) return 0;
        const char letter = (char) std::tolower ((unsigned char) m[1].str()[0]);
        const int base = letter == 'c' ? 0 : letter == 'd' ? 2 : letter == 'e' ? 4
                       : letter == 'f' ? 5 : letter == 'g' ? 7 : letter == 'a' ? 9 : 11;
        const auto acc = m[2].str();
        const int accidental = (acc == "#" || acc == "s") ? 1 : acc == "b" ? -1 : 0;
        const int octave = std::atoi (m[3].str().c_str());
        return std::min (96, std::max (12, (octave + 1) * 12 + base + accidental));
    }

    inline int drumLane (const std::string& token)
    {
        std::string t;
        for (const char c : token) t += (char) std::tolower ((unsigned char) c);
        if (t == "bd" || t == "kick") return 0;
        if (t == "sd" || t == "snare") return 1;
        if (t == "hh" || t == "oh" || t == "ch") return 2;
        if (t == "cp" || t == "clap") return 3;
        return -1;
    }

    /** Parses the documented subset: setcps/setbpm, note("...") rows, single-sound s("...") rows. */
    inline ImportResult importCode (std::string text)
    {
        ImportResult result;
        text = trim (text);

        // strudel.cc long URL carries the code itself after '#'.
        if (text.find ("strudel.cc") != std::string::npos)
        {
            const auto hashPos = text.find ('#');
            const std::string hash = hashPos == std::string::npos ? "" : text.substr (hashPos + 1);
            text = hash.empty() ? "" : base64Decode (urlDecode (hash));
        }

        static const std::regex bpmRe ("\\b(setbpm|setcps)\\s*\\(\\s*([0-9]+(?:\\.[0-9]+)?(?:\\s*/\\s*[0-9]+(?:\\.[0-9]+)?)*)\\s*\\)");
        std::smatch m;
        if (std::regex_search (text, m, bpmRe))
        {
            float beats;
            if (parseNumber (m[2].str(), beats))
            {
                const float raw = m[1].str() == "setbpm" ? beats : beats * 240.0f;
                result.bpm = std::min (300.0f, std::max (40.0f, raw));
                result.hasBpm = true;
                result.applied.push_back ("tempo");
            }
        }

        result.pattern.clear();
        static const std::regex rowRe ("\\b(note|s|sound)\\s*\\(\\s*\"([^\"]*)\"");
        std::vector<std::pair<std::string, std::string>> rows;
        for (std::sregex_iterator it (text.begin(), text.end(), rowRe), end; it != end; ++it)
            rows.emplace_back ((*it)[1].str(), (*it)[2].str());

        int noteRow = 0;
        for (const auto& row : rows)
        {
            if (row.first != "note") continue;
            if (noteRow >= 2) break;
            auto* lane = noteRow == 0 ? result.pattern.acidA : result.pattern.acidB;
            const auto tokens = splitWs (trim (row.second));
            if (tokens.size() > 16)
                result.skipped.push_back ("note row " + std::to_string (noteRow + 1)
                                          + " trimmed from " + std::to_string (tokens.size()) + " to 16 steps");
            bool anyNote = false;
            for (size_t i = 0; i < 16 && i < tokens.size(); ++i)
            {
                const int note = parseNoteToken (tokens[i]);
                if (note == REST) {}
                else if (note != 0) { lane[i].active = true; lane[i].note = note; anyNote = true; }
                else result.skipped.push_back ("unsupported note token '" + tokens[i] + "'");
            }
            if (anyNote)
                result.applied.push_back (noteRow == 0 ? "ACID A notes" : "ACID B notes");
            ++noteRow;
        }

        int laneNext[4] = { 0, 0, 0, 0 }; // next drum machine slot per instrument lane
        static const char* const drumNames[4] = { "bd", "sd", "hh", "cp" };
        for (const auto& row : rows)
        {
            if (row.first != "s" && row.first != "sound") continue;
            const auto tokens = splitWs (trim (row.second));
            std::vector<std::string> hits;
            for (const auto& t : tokens) if (t != "~") hits.push_back (t);
            int lane = -1;
            bool ambiguous = false;
            for (const auto& hit : hits)
            {
                const int l = drumLane (hit);
                if (l < 0) { ambiguous = true; break; }
                if (lane < 0) lane = l;
                else if (lane != l) { ambiguous = true; break; }
            }
            if (lane < 0 || ambiguous)
            {
                if (! hits.empty())
                {
                    std::string preview;
                    for (size_t i = 0; i < hits.size() && i < 3; ++i) { if (i) preview += " "; preview += hits[i]; }
                    result.skipped.push_back ("sound row '" + preview + "' mixes or uses unsupported sounds");
                }
                continue;
            }
            const int machine = laneNext[lane];
            if (machine > 1)
            {
                result.skipped.push_back ("extra " + std::string (drumNames[lane]) + " row beyond two drum machines");
                continue;
            }
            ++laneNext[lane];
            if (tokens.size() > 16)
                result.skipped.push_back (std::string (drumNames[lane]) + " row trimmed from "
                                          + std::to_string (tokens.size()) + " to 16 steps");
            auto& d = machine == 0 ? result.pattern.drum8 : result.pattern.drum9;
            bool* target = lane == 0 ? d.kick : lane == 1 ? d.snare : lane == 2 ? d.hat : d.clap;
            bool anyHit = false;
            for (size_t i = 0; i < 16 && i < tokens.size(); ++i)
            {
                if (tokens[i] == "~") {}
                else if (drumLane (tokens[i]) >= 0) { target[i] = true; anyHit = true; }
                else result.skipped.push_back ("unsupported sound token '" + tokens[i] + "'");
            }
            if (anyHit)
                result.applied.push_back ((machine == 0 ? "DRUM 8 " : "DRUM 9 ") + std::string (drumNames[lane]));
        }

        for (const auto& a : result.applied)
            if (a != "tempo") result.hasPattern = true;
        return result;
    }
}

} // namespace pulseforge
