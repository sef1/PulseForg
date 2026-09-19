package com.sefi.pulseforge

import org.junit.Assert.*
import org.junit.Test

class PatternBankStoreTest {
    @Test fun distinctBanksRoundTripWithoutLosingStepsOrAccent() {
        val edit=Pattern(); val banks=PatternBankStore(edit)
        edit.acidA[2].apply{active=true;velocity=2;accent=true;note=41}; edit.drum8.kick[3]=true
        banks.store(edit)
        banks.switchTo(1,edit)
        assertFalse(edit.acidA[2].active); assertFalse(edit.drum8.kick[3])
        edit.acidB[7].apply{active=true;velocity=1;note=33}; edit.drum9.clap[12]=true
        banks.store(edit)
        banks.switchTo(0,edit)
        assertTrue(edit.acidA[2].active); assertTrue(edit.acidA[2].accent); assertEquals(2,edit.acidA[2].velocity); assertTrue(edit.drum8.kick[3])
        assertFalse(edit.acidB[7].active)
        banks.switchTo(1,edit)
        assertTrue(edit.acidB[7].active); assertEquals(33,edit.acidB[7].note); assertTrue(edit.drum9.clap[12]); assertFalse(edit.acidA[2].active)
    }

    @Test fun allBanksSurvivePersistenceEncoding() {
        val edit=Pattern(); val first=PatternBankStore(edit)
        edit.acidA[0].apply{active=true;accent=true};first.store(edit)
        first.switchTo(4,edit);edit.acidB[15].apply{active=true;velocity=2;slide=true};first.store(edit)
        val second=PatternBankStore(Pattern());assertTrue(second.restore(first.encode()))
        second.switchTo(0,edit);assertTrue(edit.acidA[0].accent)
        second.switchTo(4,edit);assertTrue(edit.acidB[15].active);assertTrue(edit.acidB[15].slide);assertEquals(2,edit.acidB[15].velocity)
    }
}
