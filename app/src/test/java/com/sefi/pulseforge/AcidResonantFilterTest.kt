package com.sefi.pulseforge

import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.math.abs

class AcidResonantFilterTest {
    private fun tailEnergy(resonance: Double, sampleRate: Double): Double {
        val f=AcidResonantFilter(sampleRate); var energy=0.0; var peak=0.0
        repeat((sampleRate*.25).toInt()) { i ->
            val y=f.process(if(i<16)1.0 else 0.0,.57,1.0,resonance)
            assertTrue(y.isFinite()); peak=maxOf(peak,abs(y))
            if(i>(sampleRate*.025).toInt()) energy+=y*y
        }
        assertTrue(peak<=1.000001); return energy
    }
    @Test fun highResonanceHasLongStableRingingTail() {
        for(sr in doubleArrayOf(44100.0,48000.0,96000.0))
            assertTrue(tailEnergy(1.0,sr)>tailEnergy(.05,sr)*20.0)
    }
}
