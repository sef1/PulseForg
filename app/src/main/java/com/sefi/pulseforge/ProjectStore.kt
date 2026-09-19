package com.sefi.pulseforge

import org.json.JSONArray
import org.json.JSONObject

/** Full-project save/load: every pattern bank, live bank, BPM, synth and mixer settings. */
object ProjectStore {
    const val VERSION = 1

    fun encode(bpm: Float, bank: Int, banks: PatternBankStore, synth: Array<SynthParams>, mixer: Array<MixerChannel>): String {
        val root = JSONObject()
        root.put("app", "pulseforge")
        root.put("version", VERSION)
        root.put("bpm", bpm.toDouble())
        root.put("bank", bank)
        root.put("banks", banks.encode())
        val synths = JSONArray()
        synth.forEach { p ->
            synths.put(JSONObject()
                .put("tune", p.tune.toDouble()).put("cutoff", p.cutoff.toDouble()).put("resonance", p.resonance.toDouble())
                .put("envMod", p.envMod.toDouble()).put("decay", p.decay.toDouble()).put("accentAmount", p.accentAmount.toDouble())
                .put("squareWave", p.squareWave).put("extendedEnvelope", p.extendedEnvelope)
                .put("attack", p.attack.toDouble()).put("sustain", p.sustain.toDouble()).put("release", p.release.toDouble())
                .put("slideTime", p.slideTime.toDouble()).put("accentDecay", p.accentDecay.toDouble()))
        }
        root.put("synth", synths)
        val mixers = JSONArray()
        mixer.forEach { m -> mixers.put(JSONObject().put("pan", m.pan.toDouble()).put("send", m.send.toDouble()).put("level", m.level.toDouble())) }
        root.put("mixer", mixers)
        return root.toString()
    }

    /** Returns null when the document is not a PulseForge project. */
    fun decode(json: String): Project? = try {
        val root = JSONObject(json)
        if (root.optString("app") != "pulseforge") null
        else {
            val synths = root.getJSONArray("synth")
            val mixers = root.getJSONArray("mixer")
            if (synths.length() != 2 || mixers.length() != 4) null
            else Project(
                bpm = root.getDouble("bpm").toFloat().coerceIn(40f, 300f),
                bank = root.getInt("bank").coerceIn(0, 7),
                banksEncoded = root.getString("banks"),
                synth = (0 until 2).map { i ->
                    val o = synths.getJSONObject(i)
                    SynthParams().apply {
                        tune = f(o, "tune", tune); cutoff = f(o, "cutoff", cutoff); resonance = f(o, "resonance", resonance)
                        envMod = f(o, "envMod", envMod); decay = f(o, "decay", decay); accentAmount = f(o, "accentAmount", accentAmount)
                        squareWave = o.optBoolean("squareWave", squareWave); extendedEnvelope = o.optBoolean("extendedEnvelope", extendedEnvelope)
                        attack = f(o, "attack", attack); sustain = f(o, "sustain", sustain); release = f(o, "release", release)
                        slideTime = f(o, "slideTime", slideTime); accentDecay = f(o, "accentDecay", accentDecay)
                    }
                }.toTypedArray(),
                mixer = (0 until 4).map { i ->
                    val o = mixers.getJSONObject(i)
                    MixerChannel().apply { pan = f(o, "pan", pan); send = f(o, "send", send); level = f(o, "level", level) }
                }.toTypedArray()
            )
        }
    } catch (_: Exception) { null }

    private fun f(o: JSONObject, key: String, fallback: Float) = o.optDouble(key, fallback.toDouble()).toFloat().coerceIn(0f, 1f)

    data class Project(
        val bpm: Float,
        val bank: Int,
        val banksEncoded: String,
        val synth: Array<SynthParams>,
        val mixer: Array<MixerChannel>
    )
}
