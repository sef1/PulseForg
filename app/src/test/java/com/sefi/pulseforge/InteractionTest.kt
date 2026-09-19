package com.sefi.pulseforge

import android.view.MotionEvent
import androidx.test.core.app.ApplicationProvider
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk=[34])
class InteractionTest {
    private fun view()=GrooveboxView(ApplicationProvider.getApplicationContext()).apply{measure(ViewSpec,ViewSpecH);layout(0,0,1080,1920)}
    private fun drag(v:GrooveboxView,x:Float,y:Float,toY:Float){v.dispatchTouchEvent(MotionEvent.obtain(0,0,MotionEvent.ACTION_DOWN,x,y,0));v.dispatchTouchEvent(MotionEvent.obtain(0,20,MotionEvent.ACTION_MOVE,x,toY,0));v.dispatchTouchEvent(MotionEvent.obtain(0,30,MotionEvent.ACTION_UP,x,toY,0))}
    @Test fun synthKnobDragChangesAudioParameter(){val v=view();val before=v.engine.synth[0].cutoff;drag(v,231f,342f,220f);assertTrue(v.engine.synth[0].cutoff>before)}
    @Test fun mixerKnobAndFaderChangeParameters(){val v=view();val pan=v.engine.mixer[0].pan;drag(v,166f,1432f,1320f);assertTrue(v.engine.mixer[0].pan>pan);drag(v,195f,1580f,1490f);assertTrue(v.engine.mixer[0].level>.85f)}
    @Test fun stepCyclesLowHighOffAndAccentEditTogglesAccent(){val v=view();val s=v.pattern.acidA[1];assertFalse(s.active);tap(v,150f,485f);assertTrue(s.active);assertEquals(1,s.velocity);tap(v,150f,485f);assertEquals(2,s.velocity);tap(v,150f,485f);assertFalse(s.active);tap(v,150f,485f);tap(v,780f,235f);tap(v,150f,485f);assertTrue(s.accent)}
    @Test fun waveformAdsrAndStepEditorButtonsRouteState(){val v=view();assertFalse(v.engine.synth[0].squareWave);tap(v,380f,235f);assertTrue(v.engine.synth[0].squareWave);tap(v,500f,235f);assertTrue(v.engine.synth[0].extendedEnvelope);tap(v,620f,235f);tap(v,570f,326f);assertTrue(v.pattern.acidA[0].active);val before=v.pattern.acidA[0].note;tap(v,300f,395f);assertEquals(before+12,v.pattern.acidA[0].note);tap(v,500f,395f);assertTrue(v.pattern.acidA[0].accent);tap(v,700f,395f);assertTrue(v.pattern.acidA[0].slide)}
    @Test fun advancedTimingControlsAreIndependentAndPersist(){
        val context=ApplicationProvider.getApplicationContext<android.content.Context>()
        context.getSharedPreferences("synth",android.content.Context.MODE_PRIVATE).edit().clear().commit()
        val v=view();tap(v,950f,235f)
        val bSlide=v.engine.synth[1].slideTime;val aAccent=v.engine.synth[0].accentDecay
        drag(v,367f,342f,250f);drag(v,713f,342f,285f)
        assertTrue(v.engine.synth[0].slideTime>.30f);assertTrue(v.engine.synth[0].accentDecay>.35f)
        assertEquals(bSlide,v.engine.synth[1].slideTime,0f)
        val savedSlide=v.engine.synth[0].slideTime;val savedAccent=v.engine.synth[0].accentDecay
        val restored=view();assertEquals(savedSlide,restored.engine.synth[0].slideTime,0f);assertEquals(savedAccent,restored.engine.synth[0].accentDecay,0f);assertEquals(aAccent,restored.engine.synth[1].accentDecay,0f)
    }
    @Test fun engineBStepEditKeysStripAndTogglesRespondToTouches(){
        val context=ApplicationProvider.getApplicationContext<android.content.Context>()
        context.getSharedPreferences("synth",android.content.Context.MODE_PRIVATE).edit().clear().commit()
        val v=view()
        tap(v,380f,628f);assertTrue(v.engine.synth[1].squareWave)
        tap(v,500f,628f);assertTrue(v.engine.synth[1].extendedEnvelope)
        tap(v,620f,628f)
        val s0=v.pattern.acidB[0]
        tap(v,122f,700f);assertTrue(s0.active);assertEquals(24,s0.note)
        tap(v,392f,850f);assertTrue(v.pattern.acidB[5].active)
        tap(v,220f,700f);assertEquals(26,v.pattern.acidB[5].note)
        tap(v,551f,770f);assertTrue(v.pattern.acidB[5].accent)
        tap(v,744f,770f);assertTrue(v.pattern.acidB[5].slide)
    }
    @Test fun engineBAdvancedKnobsDragVisiblyAndPersist(){
        val context=ApplicationProvider.getApplicationContext<android.content.Context>()
        context.getSharedPreferences("synth",android.content.Context.MODE_PRIVATE).edit().clear().commit()
        val v=view();tap(v,920f,628f)
        val aSlide=v.engine.synth[0].slideTime;val aAccent=v.engine.synth[0].accentDecay
        drag(v,376f,730f,650f);drag(v,704f,730f,665f)
        assertTrue(v.engine.synth[1].slideTime>.30f);assertTrue(v.engine.synth[1].accentDecay>.35f)
        assertEquals(aSlide,v.engine.synth[0].slideTime,0f);assertEquals(aAccent,v.engine.synth[0].accentDecay,0f)
        val savedSlide=v.engine.synth[1].slideTime;val savedAccent=v.engine.synth[1].accentDecay
        val restored=view();assertEquals(savedSlide,restored.engine.synth[1].slideTime,0f);assertEquals(savedAccent,restored.engine.synth[1].accentDecay,0f)
    }
    @Test fun mixerMuteSoloButtonsToggleAndIsolate(){
        val v=view()
        tap(v,308f,1533f);assertTrue(v.engine.mixer[0].mute)
        tap(v,308f,1533f);assertFalse(v.engine.mixer[0].mute)
        tap(v,308f,1587f);assertTrue(v.engine.mixer[0].solo)
        tap(v,519f,1533f);assertTrue(v.engine.mixer[1].mute)
        assertTrue(v.engine.channelAudible(0));assertFalse(v.engine.channelAudible(1))
    }
    @Test fun muteSilencesAndSoloIsolatesInModel(){
        val e=SynthEngine(Pattern()){}
        assertTrue(e.channelAudible(0))
        e.mixer[2].mute=true;assertFalse(e.channelAudible(2))
        e.mixer[0].solo=true
        assertTrue(e.channelAudible(0));assertFalse(e.channelAudible(1));assertFalse(e.channelAudible(3))
        e.mixer[1].solo=true;assertTrue(e.channelAudible(1))
        e.mixer[0].mute=true;assertFalse(e.channelAudible(0))
        e.mixer[0].solo=false;e.mixer[1].solo=false
        assertFalse(e.channelAudible(0));assertTrue(e.channelAudible(1))
        e.mixer[2].mute=false;assertTrue(e.channelAudible(2))
    }
    private fun tap(v:GrooveboxView,x:Float,y:Float){v.dispatchTouchEvent(MotionEvent.obtain(0,0,MotionEvent.ACTION_DOWN,x,y,0));v.dispatchTouchEvent(MotionEvent.obtain(0,10,MotionEvent.ACTION_UP,x,y,0))}
    companion object {val ViewSpec=android.view.View.MeasureSpec.makeMeasureSpec(1080,android.view.View.MeasureSpec.EXACTLY);val ViewSpecH=android.view.View.MeasureSpec.makeMeasureSpec(1920,android.view.View.MeasureSpec.EXACTLY)}
}
