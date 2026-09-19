package com.sefi.pulseforge

import android.util.Base64
import java.net.URLDecoder
import java.net.URLEncoder

/**
 * Bridge to Strudel (https://strudel.cc), the AGPL-3.0 live-coding environment.
 * PulseForge never embeds Strudel code; it converts its own pattern data to and
 * from Strudel pattern text, and opens playback through a documented strudel.cc
 * long URL (code2hash: encodeURIComponent(base64(code)) after '#').
 */
object StrudelBridge {
    private val drumNames = arrayOf("bd", "sd", "hh", "cp")

    fun exportCode(pattern: Pattern, bpm: Float, waveA: Boolean, waveB: Boolean): String {
        val sb = StringBuilder()
        sb.append("// PulseForge export - accents, slides, velocity and filter settings have no Strudel equivalent here\n")
        sb.append("setcps(").append(trim(bpm / 240f)).append(")\n")
        sb.append("stack(\n")
        sb.append("  note(\"").append(acidTokens(pattern.acidA)).append("\")")
        sb.append(".s(\"").append(if (waveA) "square" else "sawtooth").append("\"),\n")
        sb.append("  note(\"").append(acidTokens(pattern.acidB)).append("\")")
        sb.append(".s(\"").append(if (waveB) "square" else "sawtooth").append("\")")
        listOf(pattern.drum8 to "DRUM 8", pattern.drum9 to "DRUM 9").forEach { (d, label) ->
            val lanes = arrayOf(d.kick, d.snare, d.hat, d.clap)
            for (lane in 0..3) {
                sb.append(",\n  // ").append(label).append(' ').append(drumNames[lane]).append('\n')
                sb.append("  s(\"").append(laneTokens(lanes[lane], drumNames[lane])).append("\")")
            }
        }
        sb.append("\n)\n")
        return sb.toString()
    }

    /** Long strudel.cc URL that carries the code itself and needs no share database. */
    fun exportUrl(code: String): String {
        val b64 = Base64.encodeToString(code.toByteArray(Charsets.UTF_8), Base64.NO_WRAP)
        return "https://strudel.cc/#" + URLEncoder.encode(b64, "UTF-8")
    }

    private fun acidTokens(steps: Array<Step>) = steps.joinToString(" ") { if (it.active) it.note.toString() else "~" }
    private fun laneTokens(lane: BooleanArray, name: String) = lane.joinToString(" ") { if (it) name else "~" }
    private fun trim(v: Float): String = if (v == v.toLong().toFloat()) v.toLong().toString() else v.toString()

    data class ImportResult(
        val pattern: Pattern?,
        val bpm: Float?,
        val applied: List<String>,
        val skipped: List<String>
    )

    /** Parses a bounded, documented subset: setcps/setbpm, note("...") rows, and single-sound s("...") rows. */
    fun importCode(input: String): ImportResult {
        var text = input.trim()
        if (text.contains("strudel.cc")) {
            val hash = text.substringAfter('#', "")
            text = if (hash.isNotEmpty()) try {
                String(Base64.decode(URLDecoder.decode(hash, "UTF-8"), Base64.DEFAULT), Charsets.UTF_8)
            } catch (_: Exception) { "" } else ""
        }
        val applied = mutableListOf<String>()
        val skipped = mutableListOf<String>()
        var bpm: Float? = null
        BPM_RE.find(text)?.let { m ->
            parseNumber(m.groupValues[2])?.let { beats ->
                bpm = (if (m.groupValues[1] == "setbpm") beats else beats * 240f).coerceIn(40f, 300f)
                applied.add("tempo")
            }
        }
        val pattern = Pattern()
        pattern.clear()
        ROW_RE.findAll(text).filter { it.groupValues[1] == "note" }.take(2).forEachIndexed { index, row ->
            val lane = if (index == 0) pattern.acidA else pattern.acidB
            val tokens = row.groupValues[2].trim().split(WS).filter { it.isNotEmpty() }
            if (tokens.size > 16) skipped.add("note row ${index + 1} trimmed from ${tokens.size} to 16 steps")
            var anyNote = false
            for (i in 0 until minOf(16, tokens.size)) {
                val note = parseNoteToken(tokens[i])
                when {
                    note == REST -> {}
                    note != null -> { lane[i].active = true; lane[i].note = note; anyNote = true }
                    else -> skipped.add("unsupported note token '${tokens[i]}'")
                }
            }
            if (anyNote) applied.add(if (index == 0) "ACID A notes" else "ACID B notes")
        }
        val laneNext = IntArray(4) // next drum machine slot per instrument lane
        ROW_RE.findAll(text).filter { it.groupValues[1] == "s" || it.groupValues[1] == "sound" }.forEach { row ->
            val tokens = row.groupValues[2].trim().split(WS).filter { it.isNotEmpty() }
            val hits = tokens.filter { it != "~" }
            val lane = hits.mapNotNull { drumLane(it) }.distinct().singleOrNull()
            if (lane == null) {
                if (hits.isNotEmpty()) skipped.add("sound row '${hits.take(3).joinToString(" ")}' mixes or uses unsupported sounds")
                return@forEach
            }
            val machine = laneNext[lane]
            if (machine > 1) { skipped.add("extra ${drumNames[lane]} row beyond two drum machines"); return@forEach }
            laneNext[lane]++
            if (tokens.size > 16) skipped.add("${drumNames[lane]} row trimmed from ${tokens.size} to 16 steps")
            val d = if (machine == 0) pattern.drum8 else pattern.drum9
            val target = when (lane) { 0 -> d.kick; 1 -> d.snare; 2 -> d.hat; else -> d.clap }
            var anyHit = false
            for (i in 0 until minOf(16, tokens.size)) {
                when {
                    tokens[i] == "~" -> {}
                    drumLane(tokens[i]) != null -> { target[i] = true; anyHit = true }
                    else -> skipped.add("unsupported sound token '${tokens[i]}'")
                }
            }
            if (anyHit) applied.add((if (machine == 0) "DRUM 8 " else "DRUM 9 ") + drumNames[lane])
        }
        val hasContent = applied.any { it != "tempo" }
        return ImportResult(if (hasContent) pattern else null, bpm, applied, skipped)
    }

    private val REST = -1
    private val WS = Regex("\\s+")
    private val BPM_RE = Regex("\\b(setbpm|setcps)\\s*\\(\\s*([0-9]+(?:\\.[0-9]+)?(?:\\s*/\\s*[0-9]+(?:\\.[0-9]+)?)*)\\s*\\)")
    private val ROW_RE = Regex("\\b(note|s|sound)\\s*\\(\\s*\"([^\"]*)\"")

    private fun parseNumber(expr: String): Float? {
        var value: Float? = null
        for (part in expr.split('/')) {
            val n = part.trim().toFloatOrNull() ?: return null
            if (n == 0f && value != null) return null
            value = if (value == null) n else value / n
        }
        return value
    }

    private fun parseNoteToken(token: String): Int? {
        if (token == "~") return REST
        if (token.any { it in "*[]<>,!?@{}|\\" }) return null
        token.toIntOrNull()?.let { return if (it in 0..127) it.coerceIn(12, 96) else null }
        val m = Regex("^([a-gA-G])([#sb]?)(-?[0-9])?$").matchEntire(token) ?: return null
        val base = when (m.groupValues[1].lowercase()) { "c" -> 0; "d" -> 2; "e" -> 4; "f" -> 5; "g" -> 7; "a" -> 9; else -> 11 }
        val accidental = when (m.groupValues[2]) { "#", "s" -> 1; "b" -> -1; else -> 0 }
        val octave = m.groupValues[3].toIntOrNull() ?: return null
        return ((octave + 1) * 12 + base + accidental).coerceIn(12, 96)
    }

    private fun drumLane(token: String): Int? = when (token.lowercase()) {
        "bd", "kick" -> 0
        "sd", "snare" -> 1
        "hh", "oh", "ch" -> 2
        "cp", "clap" -> 3
        else -> null
    }
}
