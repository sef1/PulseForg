package com.sefi.pulseforge
import org.junit.Assert.*
import org.junit.Test
class AcidStepEditorTest {
 @Test fun pitchKeysAndTransposeRouteToSelectedStep(){val s=Step(note=36);AcidStepEditor.setPitch(s,4);assertEquals(43,s.note);AcidStepEditor.transpose(s,12);assertEquals(55,s.note);AcidStepEditor.transpose(s,-12);assertEquals(43,s.note);assertTrue(s.active)}
 @Test fun accentAndSlideRemainIndependent(){val s=Step();AcidStepEditor.toggleAccent(s);assertTrue(s.accent);assertFalse(s.slide);AcidStepEditor.toggleSlide(s);assertTrue(s.accent);assertTrue(s.slide);AcidStepEditor.toggleAccent(s);assertFalse(s.accent);assertTrue(s.slide)}
}
