package com.sefi.pulseforge

import kotlin.math.exp

/**
 * Original clean-room approximation of the coupled accent dynamics found in
 * classic acid basslines. Accent is deliberately separate from velocity.
 * State is retained between steps so closely repeated accents build a higher
 * filter sweep instead of behaving like isolated volume boosts.
 */
class AcidAccentModel(private val sampleRate: Int = 44100) {
    private var sweepCharge = 0.0

    fun beginStep(accented: Boolean, amount: Double) {
        if (accented) {
            // A fresh trigger charges an already partly charged sweep state.
            sweepCharge = (sweepCharge + 0.58 + amount.coerceIn(0.0, 1.0) * 0.72).coerceAtMost(2.35)
        }
    }

    fun sample(
        secondsIntoStep: Double,
        accented: Boolean,
        accentAmount: Double,
        resonance: Double,
        normalDecay: Double,
        accentDecay: Double = 0.35
    ): AcidAccentFrame {
        val amount = accentAmount.coerceIn(0.0, 1.0)
        // Accent Decay is independent of the normal DECAY/ADSR controls.
        // Its 0..1 range maps to about 35..300 ms.
        val accentSeconds = 0.035 + accentDecay.coerceIn(0.0, 1.0) * 0.265
        val filterRate = if (accented) 1.0 / accentSeconds else 2.0 + (1.0 - normalDecay.coerceIn(0.0, 1.0)) * 15.0
        val filterEnvelope = exp(-secondsIntoStep * filterRate)

        // The VCA accent follows the short main envelope, with a softened attack.
        val attack = if (accented) 1.0 - exp(-secondsIntoStep / 0.00155) else 0.0
        val amplitudeLift = if (accented) amount * filterEnvelope * attack * 0.72 else 0.0

        // Resonance increases the audible contribution of the retained sweep.
        val resonanceCoupling = 0.70 + resonance.coerceIn(0.0, 1.0) * 0.75
        val accentSweep = if (accented) sweepCharge * amount * resonanceCoupling else 0.0

        // Roughly 170 ms memory: fast patterns retain more charge between accents.
        sweepCharge *= exp(-1.0 / (sampleRate * 0.170))
        return AcidAccentFrame(filterEnvelope, accentSweep, amplitudeLift, sweepCharge)
    }

    fun retainedSweepCharge(): Double = sweepCharge
    fun reset() { sweepCharge = 0.0 }
}

data class AcidAccentFrame(
    val filterEnvelope: Double,
    val filterSweep: Double,
    val amplitudeLift: Double,
    val retainedCharge: Double
)
