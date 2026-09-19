package com.sefi.pulseforge

import androidx.test.core.app.ApplicationProvider
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk=[34])
class ProjectStoreTest {
    @Test fun fullProjectRoundTripsThroughJson(){
        val view=GrooveboxView(ApplicationProvider.getApplicationContext())
        view.pattern.acidA[4].apply{active=true;note=52;accent=true;slide=true}
        view.pattern.drum9.clap[7]=true
        view.engine.bpm=133f
        view.engine.synth[1].slideTime=.77f;view.engine.synth[1].squareWave=true
        view.engine.mixer[2].level=.33f;view.engine.mixer[3].pan=.91f
        val json=view.exportProjectJson()

        val fresh=GrooveboxView(ApplicationProvider.getApplicationContext())
        assertTrue(fresh.importProjectJson(json))
        assertEquals(133f,fresh.engine.bpm,0f)
        assertTrue(fresh.pattern.acidA[4].active);assertEquals(52,fresh.pattern.acidA[4].note)
        assertTrue(fresh.pattern.acidA[4].accent);assertTrue(fresh.pattern.acidA[4].slide)
        assertTrue(fresh.pattern.drum9.clap[7])
        assertEquals(.77f,fresh.engine.synth[1].slideTime,0f);assertTrue(fresh.engine.synth[1].squareWave)
        assertEquals(.33f,fresh.engine.mixer[2].level,0f);assertEquals(.91f,fresh.engine.mixer[3].pan,0f)
    }
    @Test fun foreignJsonIsRejected(){
        assertNull(ProjectStore.decode("{\"hello\":\"world\"}"))
        assertNull(ProjectStore.decode("not json"))
        val view=GrooveboxView(ApplicationProvider.getApplicationContext())
        assertFalse(view.importProjectJson("{\"app\":\"other\",\"version\":1}"))
    }
}
