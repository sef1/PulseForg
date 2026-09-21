package com.sefi.pulseforge

import kotlin.math.PI
import kotlin.math.min
import kotlin.math.max
import kotlin.math.pow
import kotlin.math.tan
import kotlin.math.tanh

/**
 * Non-linear TPT state-variable low-pass shared with the VST acid engine.
 * The response stays stable during audio-rate cutoff sweeps and enters a
 * high-Q ringing region in the top quarter of the RESO control.
 */
class AcidResonantFilter(private val sampleRate: Double) {
    private var ic1eq = 0.0
    private var ic2eq = 0.0

    fun reset() { ic1eq = 0.0; ic2eq = 0.0 }

    fun process(input: Double, cutoffControl: Double, sweep: Double, resonance: Double): Double {
        val c = cutoffControl.coerceIn(0.0, 1.0)
        val hz = min(sampleRate * 0.20, max(25.0, (35.0 + c * c * 8000.0) * sweep))
        val g = tan(PI * hz / sampleRate)
        val r = resonance.coerceIn(0.0, 1.0)
        val k = 0.025 + 1.975 * (1.0 - r).pow(2.35)
        val a1 = 1.0 / (1.0 + g * (g + k))
        val driven = tanh(input * (1.0 + r * 0.85))
        val v3 = driven - ic2eq
        val v1 = a1 * ic1eq + a1 * g * v3
        val v2 = ic2eq + g * v1
        ic1eq = 2.0 * v1 - ic1eq
        ic2eq = 2.0 * v2 - ic2eq
        return tanh(v2 * (1.0 + r * 0.28))
    }
}
