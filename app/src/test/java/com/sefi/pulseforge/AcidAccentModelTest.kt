package com.sefi.pulseforge

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class AcidAccentModelTest {
    @Test fun accentIsIndependentOfVelocityAndRaisesFilterAndAmplitudeDynamics() {
        val m=AcidAccentModel(44100); m.beginStep(true, .8)
        val f=m.sample(.004,true,.8,.7,.9)
        assertTrue(f.filterSweep>0.0)
        assertTrue(f.amplitudeLift>0.0)
    }

    @Test fun accentedFilterDecayIgnoresDecayKnob() {
        val a=AcidAccentModel(); val b=AcidAccentModel()
        a.beginStep(true,.7); b.beginStep(true,.7)
        val slow=a.sample(.03,true,.7,.5,.0)
        val long=b.sample(.03,true,.7,.5,1.0)
        assertEquals(slow.filterEnvelope,long.filterEnvelope,1e-12)
    }

    @Test fun consecutiveAccentsBuildSweepHigher() {
        val m=AcidAccentModel(1000)
        m.beginStep(true,1.0); val first=m.sample(0.0,true,1.0,.8,.5).filterSweep
        repeat(125){m.sample(it/1000.0,true,1.0,.8,.5)}
        m.beginStep(true,1.0); val second=m.sample(0.0,true,1.0,.8,.5).filterSweep
        assertTrue(second>first)
    }

    @Test fun nonAccentHasNoAccentLiftAndStoredSweepDecays() {
        val m=AcidAccentModel(1000); m.beginStep(true,1.0); m.sample(0.0,true,1.0,.5,.5)
        val before=m.retainedSweepCharge(); repeat(250){m.sample(it/1000.0,false,1.0,.5,.5)}
        val plain=m.sample(.01,false,1.0,.5,.5)
        assertEquals(0.0,plain.filterSweep,0.0); assertEquals(0.0,plain.amplitudeLift,0.0)
        assertTrue(m.retainedSweepCharge()<before)
    }

    @Test fun resonanceIncreasesAccentSweepContribution() {
        val low=AcidAccentModel(); val high=AcidAccentModel()
        low.beginStep(true,.8); high.beginStep(true,.8)
        assertTrue(high.sample(0.0,true,.8,1.0,.5).filterSweep > low.sample(0.0,true,.8,0.0,.5).filterSweep)
    }
    @Test fun accentDecayChangesEnvelopeLengthWithoutNormalDecay(){
        val short=AcidAccentModel(44100);val long=AcidAccentModel(44100)
        short.beginStep(true,.8);long.beginStep(true,.8)
        val shortFrame=short.sample(.12,true,.8,.5,.5,0.0)
        val longFrame=long.sample(.12,true,.8,.5,.5,1.0)
        assertTrue(longFrame.filterEnvelope>shortFrame.filterEnvelope)
        assertTrue(longFrame.amplitudeLift>shortFrame.amplitudeLift)
    }
}
