package com.sefi.pulseforge

import kotlin.math.exp

object AcidVoiceModel {
    /** 0..1 maps to an approximately 20..500 ms glide. */
    fun slideSeconds(slideTime: Double): Double = .020 + slideTime.coerceIn(0.0, 1.0) * .480

    fun glideFrequency(from:Double,to:Double,t:Double,sliding:Boolean,slideTime:Double=.30):Double {
        if(!sliding)return to
        val duration=slideSeconds(slideTime)
        if(t>=duration)return to
        // Exponential-feeling portamento with an exact target at the selected time.
        val shaped=(1.0-exp(-4.0*t/duration))/(1.0-exp(-4.0))
        return from+(to-from)*shaped.coerceIn(0.0,1.0)
    }
    fun oscillator(phase: Double, square: Boolean): Double = if(square) { if(phase < .5) 1.0 else -1.0 } else phase*2.0-1.0
    fun amplitudeEnvelope(t:Double, stepSeconds:Double, extended:Boolean, attack:Double, decay:Double, sustain:Double, release:Double):Double {
        if(!extended)return exp(-t*(2.0+(1.0-decay.coerceIn(0.0,1.0))*15.0))
        val attackSeconds=.002+attack.coerceIn(0.0,1.0)*.118
        val decaySeconds=.025+decay.coerceIn(0.0,1.0)*.45
        val sustainLevel=.08+sustain.coerceIn(0.0,1.0)*.90
        val releaseStart=stepSeconds*.82
        val body=if(t<attackSeconds)t/attackSeconds else sustainLevel+(1.0-sustainLevel)*exp(-(t-attackSeconds)/decaySeconds)
        return if(t<=releaseStart)body else body*exp(-(t-releaseStart)/(.015+release.coerceIn(0.0,1.0)*.35))
    }
}
