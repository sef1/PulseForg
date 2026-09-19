package com.sefi.pulseforge

import android.util.Base64
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.net.URLDecoder

@RunWith(RobolectricTestRunner::class)
@Config(sdk=[34])
class StrudelBridgeTest {
    @Test fun exportProducesPlayableSubsetWithTempoNotesAndDrums(){
        val p=Pattern()
        p.acidA[0].apply{active=true;note=36};p.acidA[3].apply{active=true;note=48;accent=true;slide=true}
        p.drum8.kick[0]=true;p.drum8.kick[8]=true;p.drum9.snare[4]=true
        val code=StrudelBridge.exportCode(p,120f,false,true)
        assertTrue(code.contains("setcps(0.5)"))
        assertTrue(code.contains("note(\"36 ~ ~ 48 ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\")"))
        assertTrue(code.contains(".s(\"sawtooth\")"));assertTrue(code.contains(".s(\"square\")"))
        assertTrue(code.contains("s(\"bd ~ ~ ~ ~ ~ ~ ~ bd ~ ~ ~ ~ ~ ~ ~\")"))
        assertTrue(code.contains("s(\"~ ~ ~ ~ sd ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\")"))
    }
    @Test fun exportUrlRoundTripsThroughStrudelHashDecoding(){
        val code=StrudelBridge.exportCode(Pattern.demo(),126f,false,false)
        val url=StrudelBridge.exportUrl(code)
        assertTrue(url.startsWith("https://strudel.cc/#"))
        val decoded=String(Base64.decode(URLDecoder.decode(url.substringAfter('#'),"UTF-8"),Base64.DEFAULT),Charsets.UTF_8)
        assertEquals(code,decoded)
    }
    @Test fun importParsesNotesDrumsAndTempo(){
        val r=StrudelBridge.importCode("setbpm(140)\nnote(\"36 ~ 39 ~\")\nnote(\"c2 ~ eb2 ~\")\ns(\"bd ~ bd ~\")\ns(\"~ cp ~ cp\")")
        assertEquals(140f,r.bpm!!,0f)
        val p=r.pattern!!
        assertTrue(p.acidA[0].active);assertEquals(36,p.acidA[0].note);assertFalse(p.acidA[1].active)
        assertTrue(p.acidB[0].active);assertEquals(36,p.acidB[0].note);assertEquals(39,p.acidB[2].note)
        assertTrue(p.drum8.kick[0]);assertTrue(p.drum8.kick[2]);assertTrue(p.drum8.clap[1]);assertTrue(p.drum8.clap[3])
        assertTrue(r.applied.any{it.contains("ACID A")});assertTrue(r.applied.any{it.contains("DRUM 8 bd")})
    }
    @Test fun importFlagsUnsupportedTokensAndMixedRows(){
        val r=StrudelBridge.importCode("note(\"36 ~ 39*2 <41 43>\")\ns(\"bd sd hh cp\")")
        val p=r.pattern!!
        assertTrue(p.acidA[0].active);assertEquals(36,p.acidA[0].note)
        assertFalse(p.acidA[1].active);assertFalse(p.acidA[2].active);assertFalse(p.acidA[3].active)
        assertTrue(r.skipped.any{it.contains("39*2")});assertTrue(r.skipped.any{it.contains("<41")})
        assertTrue(r.skipped.any{it.contains("mixes")})
        assertFalse(p.drum8.kick[0])
    }
    @Test fun importDecodesStrudelCcUrl(){
        val code="setcps(0.5)\nnote(\"40 ~ ~ ~\")"
        val url=StrudelBridge.exportUrl(code)
        val r=StrudelBridge.importCode(url)
        assertEquals(120f,r.bpm!!,0f)
        assertTrue(r.pattern!!.acidA[0].active);assertEquals(40,r.pattern!!.acidA[0].note)
    }
}
