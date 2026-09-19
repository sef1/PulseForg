package com.sefi.pulseforge

import org.junit.Assert.*
import org.junit.Test

class PatternTest {
    @Test fun demoHasSixteenStepsAndFourOnFloor() {
        val p = Pattern.demo()
        assertEquals(16, p.acidA.size)
        assertArrayEquals(booleanArrayOf(true,false,false,false,true,false,false,false,true,false,false,false,true,false,false,false), p.drum8.kick)
    }
    @Test fun clearResetsEveryLane() {
        val p = Pattern.demo(); p.clear()
        assertFalse(p.acidA.any { it.active }); assertFalse(p.acidB.any { it.active })
        assertTrue(p.drum8.kick.all { !it }); assertTrue(p.drum8.snare.all { !it }); assertTrue(p.drum8.hat.all { !it }); assertTrue(p.drum8.clap.all { !it })
    }
}
