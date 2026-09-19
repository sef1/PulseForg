package com.sefi.pulseforge

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.sin

class GrooveboxView(context: Context) : View(context) {
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG)
    internal val pattern=Pattern.demo()
    internal val engine=SynthEngine(pattern){step->post{currentStep=step;invalidate()}}
    private var currentStep=-1
    private var playing=false
    internal val patternBanks=PatternBankStore(pattern)
    internal var bank=0
    private var selectedTrack=0
    private val accentEdit=booleanArrayOf(false,false)
    private val stepEdit=booleanArrayOf(false,false)
    private val advancedEdit=booleanArrayOf(false,false)
    private val selectedAcidStep=intArrayOf(0,0)
    var onProject:(()->Unit)?=null
    private var drag:Control?=null
    private var dragStartY=0f
    private var dragStartValue=0f

    private val dark=Color.rgb(15,17,18); private val steel=Color.rgb(193,198,197); private val steel2=Color.rgb(156,163,163)
    private val ink=Color.rgb(25,28,29); private val red=Color.rgb(238,55,43); private val led=Color.rgb(255,66,48)
    private val amber=Color.rgb(255,173,31); private val green=Color.rgb(100,205,117); private val violet=Color.rgb(177,83,255)

    private sealed class Control {
        data class SynthKnob(val engine:Int,val knob:Int):Control()
        data class MixerKnob(val channel:Int,val knob:Int):Control()
        data class Fader(val channel:Int):Control()
    }

    init {
        setLayerType(LAYER_TYPE_SOFTWARE,null); isFocusable=true; contentDescription="PulseForge hardware groovebox"
        context.getSharedPreferences("patterns",Context.MODE_PRIVATE).getString("banks",null)?.let {
            if(patternBanks.restore(it)){ patternBanks.switchTo(0,pattern); bank=0 }
        }
        loadSynthParams()
    }
    fun stopAudio(){engine.stop();playing=false;invalidate()}

    override fun onDraw(c:Canvas){
        super.onDraw(c);c.drawColor(dark)
        val w=width.toFloat();val h=height.toFloat();val m=w*.025f;val gap=w*.014f;var y=h*.018f
        drawHeader(c,m,y,w-m,y+h*.082f);y+=h*.092f
        val synthH=h*.176f;drawSynth(c,0,"ACID ENGINE A",m,y,w-m,y+synthH);y+=synthH+gap
        drawSynth(c,1,"ACID ENGINE B",m,y,w-m,y+synthH);y+=synthH+gap
        val drumsH=h*.215f;drawDrums(c,m,y,w-m,y+drumsH);y+=drumsH+gap
        val mixH=h*.176f;drawMixer(c,m,y,w-m,y+mixH);y+=mixH+gap
        drawFooter(c,m,y,w-m,h-h*.018f)
    }

    private fun drawHeader(c:Canvas,l:Float,t:Float,r:Float,b:Float){panel(c,l,t,r,b);text(c,"PULSEFORGE",l+(r-l)*.035f,t+(b-t)*.42f,(b-t)*.31f,ink,true);text(c,"DUAL ACID / DRUM MACHINE",l+(r-l)*.037f,t+(b-t)*.72f,(b-t)*.14f,Color.DKGRAY,true);display(c,r-(r-l)*.31f,t+(b-t)*.18f,r-(r-l)*.17f,b-(b-t)*.18f,"${engine.bpm.toInt()}","BPM");button(c,l+(r-l)*.44f,t+(b-t)*.18f,l+(r-l)*.62f,b-(b-t)*.18f,"PROJECT",steel2);button(c,r-(r-l)*.145f,t+(b-t)*.18f,r-(r-l)*.025f,b-(b-t)*.18f,if(playing)"STOP" else "PLAY",if(playing)red else green)}

    private fun drawSynth(c:Canvas,index:Int,title:String,l:Float,t:Float,r:Float,b:Float){
        panel(c,l,t,r,b);val hh=b-t;val ww=r-l
        tag(c,l+ww*.018f,t+hh*.06f,l+ww*.235f,t+hh*.25f,title,if(selectedTrack==index)red else ink)
        button(c,l+ww*.255f,t+hh*.06f,l+ww*.365f,t+hh*.25f,if(engine.synth[index].squareWave)"SQUARE" else "SAW",if(engine.synth[index].squareWave)amber else steel2)
        button(c,l+ww*.38f,t+hh*.06f,l+ww*.49f,t+hh*.25f,"ADSR",if(engine.synth[index].extendedEnvelope)green else steel2)
        button(c,l+ww*.505f,t+hh*.06f,l+ww*.65f,t+hh*.25f,"STEP EDIT",if(stepEdit[index])green else steel2)
        button(c,l+ww*.665f,t+hh*.06f,l+ww*.80f,t+hh*.25f,"ACC EDIT",if(accentEdit[index])violet else steel2)
        button(c,l+ww*.815f,t+hh*.06f,l+ww*.94f,t+hh*.25f,"ADV",if(advancedEdit[index])green else steel2)
        val p=engine.synth[index]
        if(stepEdit[index]) drawStepEditor(c,index,l,t,ww,hh) else if(advancedEdit[index]) drawAdvanced(c,index,l,t,ww,hh) else {
            val labels=if(p.extendedEnvelope)arrayOf("ATTACK","DECAY","SUSTAIN","RELEASE","ENV","ACCENT")else arrayOf("TUNE","CUTOFF","RESO","ENV","DECAY","ACCENT")
            val values=if(p.extendedEnvelope)floatArrayOf(p.attack,p.decay,p.sustain,p.release,p.envMod,p.accentAmount)else floatArrayOf(p.tune,p.cutoff,p.resonance,p.envMod,p.decay,p.accentAmount)
            for(i in labels.indices){val x=l+ww*(.075f+i*.145f);knob(c,x,t+hh*.49f,hh*.135f,values[i],labels[i])}
        }
        val sy=t+hh*.76f;val sg=ww*.008f;val sw=(ww*.91f-sg*15)/16f;val sx=l+ww*.045f
        val steps=if(index==0)pattern.acidA else pattern.acidB
        for(i in 0..15)acidStep(c,sx+i*(sw+sg),sy,sw,hh*.16f,i,steps[i])
        text(c,"LOW",l+ww*.925f,t+hh*.55f,hh*.045f,red,true);text(c,"HIGH",l+ww*.925f,t+hh*.64f,hh*.045f,amber,true);text(c,"ACC",l+ww*.925f,t+hh*.73f,hh*.045f,violet,true)
    }

    private fun drawAdvanced(c:Canvas,index:Int,l:Float,t:Float,ww:Float,hh:Float){
        val p=engine.synth[index]
        knob(c,l+ww*.34f,t+hh*.49f,hh*.16f,p.slideTime,"SLIDE TIME")
        knob(c,l+ww*.66f,t+hh*.49f,hh*.16f,p.accentDecay,"ACC DECAY")
        text(c,"${(20+p.slideTime*480).toInt()} MS",l+ww*.34f,t+hh*.665f,hh*.055f,ink,true,Paint.Align.CENTER)
        text(c,"${(35+p.accentDecay*265).toInt()} MS",l+ww*.66f,t+hh*.665f,hh*.055f,ink,true,Paint.Align.CENTER)
        text(c,"PER-ENGINE TIMING",l+ww*.50f,t+hh*.735f,hh*.045f,Color.DKGRAY,true,Paint.Align.CENTER)
    }

    private fun drawStepEditor(c:Canvas,index:Int,l:Float,t:Float,ww:Float,hh:Float){
        val names=arrayOf("C","D","E","F","G","A","B");val gap=ww*.008f;val bw=(ww*.72f-gap*6)/7f;val sx=l+ww*.045f
        for(i in names.indices)button(c,sx+i*(bw+gap),t+hh*.31f,sx+i*(bw+gap)+bw,t+hh*.49f,names[i],steel2)
        val step=if(index==0)pattern.acidA[selectedAcidStep[index]]else pattern.acidB[selectedAcidStep[index]]
        val cw=ww*.18f;val cy=t+hh*.54f;button(c,sx,cy,sx+cw,cy+hh*.15f,"OCT -",steel2);button(c,sx+cw+gap,cy,sx+cw*2+gap,cy+hh*.15f,"OCT +",steel2);button(c,sx+(cw+gap)*2,cy,sx+cw*3+gap*2,cy+hh*.15f,"ACCENT",if(step.accent)violet else steel2);button(c,sx+(cw+gap)*3,cy,sx+cw*4+gap*3,cy+hh*.15f,"SLIDE",if(step.slide)amber else steel2)
        text(c,"S${selectedAcidStep[index]+1} N${step.note}",l+ww*.91f,t+hh*.44f,hh*.06f,ink,true,Paint.Align.CENTER)
    }

    private fun drawDrums(c:Canvas,l:Float,t:Float,r:Float,b:Float){panel(c,l,t,r,b);val ww=r-l;val hh=b-t;tag(c,l+ww*.018f,t+hh*.045f,l+ww*.19f,t+hh*.19f,"DRUM 8",if(selectedTrack==2)amber else ink);tag(c,l+ww*.205f,t+hh*.045f,l+ww*.377f,t+hh*.19f,"DRUM 9",if(selectedTrack==3)amber else ink);text(c,"16-STEP RHYTHM",r-ww*.025f,t+hh*.15f,hh*.075f,Color.DKGRAY,true,Paint.Align.RIGHT);val names=arrayOf("BD","SD","CH","CP");for(row in 0..3){val yy=t+hh*(.27f+row*.17f);text(c,names[row],l+ww*.036f,yy+hh*.082f,hh*.075f,ink,true);val sg=ww*.006f;val sx=l+ww*.10f;val sw=(ww*.865f-sg*15)/16f;for(i in 0..15){val d=if(selectedTrack==3)pattern.drum9 else pattern.drum8;val active=when(row){0->d.kick[i];1->d.snare[i];2->d.hat[i];else->d.clap[i]};step(c,sx+i*(sw+sg),yy,sw,hh*.12f,i,active,if(row==0)amber else red)}}}

    private fun drawMixer(c:Canvas,l:Float,t:Float,r:Float,b:Float){panel(c,l,t,r,b);val ww=r-l;val hh=b-t;text(c,"MIXER",l+ww*.025f,t+hh*.14f,hh*.09f,ink,true);val names=arrayOf("ACID A","ACID B","DRUM 8","DRUM 9");for(i in 0..3){val x=l+ww*(.18f+i*.205f);val mc=engine.mixer[i];text(c,names[i],x,t+hh*.14f,hh*.066f,ink,true,Paint.Align.CENTER);knob(c,x-ww*.037f,t+hh*.34f,hh*.10f,mc.pan,"PAN");knob(c,x+ww*.050f,t+hh*.34f,hh*.10f,mc.send,"SEND");fader(c,x,t+hh*.50f,x,t+hh*.88f,mc.level);lamp(c,x-ww*.062f,t+hh*.80f,if(i==selectedTrack)green else Color.rgb(85,92,91));button(c,x+ww*.066f,t+hh*.49f,x+ww*.122f,t+hh*.62f,"M",if(mc.mute)red else steel2);button(c,x+ww*.066f,t+hh*.65f,x+ww*.122f,t+hh*.78f,"S",if(mc.solo)green else steel2)}}

    private fun drawFooter(c:Canvas,l:Float,t:Float,r:Float,b:Float){panel(c,l,t,r,b);val ww=r-l;val hh=b-t;text(c,"PATTERN BANK",l+ww*.025f,t+hh*.16f,hh*.095f,ink,true);display(c,r-ww*.19f,t+hh*.055f,r-ww*.025f,t+hh*.24f,"${bank+1}","MEM");val labels=arrayOf("A","B","C","D","E","F","G","H");val g=ww*.014f;val bw=(ww*.91f-g*7)/8;val x=l+ww*.045f;val by=t+hh*.32f;for(i in labels.indices)button(c,x+i*(bw+g),by,x+i*(bw+g)+bw,by+hh*.28f,labels[i],if(i==bank)red else steel2);button(c,l+ww*.045f,t+hh*.70f,l+ww*.30f,b-hh*.06f,"CLEAR",steel2);button(c,l+ww*.325f,t+hh*.70f,l+ww*.59f,b-hh*.06f,"STORE",steel2);button(c,l+ww*.615f,t+hh*.70f,r-ww*.045f,b-hh*.06f,if(playing)"STOP" else "START",if(playing)red else green)}

    private fun panel(c:Canvas,l:Float,t:Float,r:Float,b:Float){paint.shader=LinearGradient(l,t,r,b,Color.rgb(224,227,225),Color.rgb(145,151,151),Shader.TileMode.CLAMP);paint.style=Paint.Style.FILL;c.drawRoundRect(l,t,r,b,8f,8f,paint);paint.shader=null;paint.color=Color.rgb(74,79,79);paint.style=Paint.Style.STROKE;paint.strokeWidth=2f;c.drawRoundRect(l,t,r,b,8f,8f,paint);paint.style=Paint.Style.FILL;screw(c,l+9,t+9);screw(c,r-9,t+9);screw(c,l+9,b-9);screw(c,r-9,b-9)}
    private fun screw(c:Canvas,x:Float,y:Float){paint.color=Color.rgb(86,91,91);c.drawCircle(x,y,4f,paint);paint.color=Color.rgb(210,214,212);paint.strokeWidth=1.3f;c.drawLine(x-2.2f,y,x+2.2f,y,paint)}
    private fun tag(c:Canvas,l:Float,t:Float,r:Float,b:Float,s:String,color:Int){paint.color=color;c.drawRoundRect(l,t,r,b,4f,4f,paint);text(c,s,(l+r)/2,t+(b-t)*.69f,(b-t)*.43f,Color.WHITE,true,Paint.Align.CENTER)}
    private fun text(c:Canvas,s:String,x:Float,y:Float,size:Float,color:Int,bold:Boolean=false,align:Paint.Align=Paint.Align.LEFT){paint.color=color;paint.textSize=size;paint.textAlign=align;paint.typeface=Typeface.create("sans",if(bold)Typeface.BOLD else Typeface.NORMAL);paint.style=Paint.Style.FILL;c.drawText(s,x,y,paint);paint.textAlign=Paint.Align.LEFT}
    private fun display(c:Canvas,l:Float,t:Float,r:Float,b:Float,value:String,label:String){paint.color=Color.rgb(20,14,13);c.drawRoundRect(l,t,r,b,4f,4f,paint);paint.setShadowLayer(8f,0f,0f,led);text(c,value,(l+r)/2,t+(b-t)*.66f,(b-t)*.58f,led,true,Paint.Align.CENTER);paint.clearShadowLayer();text(c,label,(l+r)/2,b+(b-t)*.22f,(b-t)*.18f,ink,true,Paint.Align.CENTER)}
    private fun button(c:Canvas,l:Float,t:Float,r:Float,b:Float,s:String,color:Int){paint.color=Color.rgb(72,76,76);c.drawRoundRect(l,t,r,b,5f,5f,paint);paint.color=color;c.drawRoundRect(l+3,t+3,r-3,b-4,4f,4f,paint);text(c,s,(l+r)/2,t+(b-t)*.65f,(b-t)*.30f,if(color==red||color==green||color==violet)Color.WHITE else ink,true,Paint.Align.CENTER)}
    private fun knob(c:Canvas,x:Float,y:Float,r:Float,v:Float,label:String){paint.color=Color.rgb(51,55,55);c.drawCircle(x,y,r,paint);paint.color=Color.rgb(104,109,108);c.drawCircle(x,y,r*.73f,paint);val a=Math.toRadians((135+270*v).toDouble());paint.color=Color.WHITE;paint.strokeWidth=maxOf(2f,r*.09f);c.drawLine(x,y,x+(cos(a)*r*.64).toFloat(),y+(sin(a)*r*.64).toFloat(),paint);text(c,label,x,y+r*1.55f,r*.48f,ink,true,Paint.Align.CENTER)}
    private fun acidStep(c:Canvas,x:Float,y:Float,w:Float,h:Float,i:Int,s:Step){paint.color=when{ i==currentStep->green;!s.active->Color.rgb(77,82,81);s.velocity==2->amber;else->red};c.drawRoundRect(x,y,x+w,y+h,3f,3f,paint);if(s.accent){paint.style=Paint.Style.STROKE;paint.strokeWidth=maxOf(3f,w*.10f);paint.color=violet;c.drawRoundRect(x+1,y+1,x+w-1,y+h-1,3f,3f,paint);paint.style=Paint.Style.FILL;paint.color=violet;c.drawCircle(x+w*.78f,y+h*.22f,maxOf(2f,h*.09f),paint)};text(c,"${i+1}",x+w/2,y+h*.68f,h*.40f,if(s.active||i==currentStep)Color.WHITE else Color.LTGRAY,true,Paint.Align.CENTER)}
    private fun step(c:Canvas,x:Float,y:Float,w:Float,h:Float,i:Int,active:Boolean,on:Int=red){paint.color=if(i==currentStep)green else if(active)on else Color.rgb(77,82,81);c.drawRoundRect(x,y,x+w,y+h,3f,3f,paint);text(c,"${i+1}",x+w/2,y+h*.68f,h*.40f,if(active||i==currentStep)Color.WHITE else Color.LTGRAY,true,Paint.Align.CENTER)}
    private fun fader(c:Canvas,x1:Float,y1:Float,x2:Float,y2:Float,v:Float){paint.color=Color.rgb(55,59,59);paint.strokeWidth=7f;c.drawLine(x1,y1,x2,y2,paint);val y=y2-(y2-y1)*v;paint.color=Color.rgb(232,234,231);c.drawRoundRect(x1-18,y-8,x1+18,y+8,3f,3f,paint);paint.color=ink;paint.strokeWidth=2f;c.drawLine(x1-15,y,x1+15,y,paint)}
    private fun lamp(c:Canvas,x:Float,y:Float,color:Int){paint.color=color;c.drawCircle(x,y,7f,paint)}

    override fun onTouchEvent(e:MotionEvent):Boolean {
        when(e.actionMasked){
            MotionEvent.ACTION_DOWN->{parent?.requestDisallowInterceptTouchEvent(true);drag=findControl(e.x,e.y);if(drag!=null){dragStartY=e.y;dragStartValue=controlValue(drag!!);updateControl(drag!!,e.x,e.y,false)}else handleTap(e.x,e.y);invalidate();return true}
            MotionEvent.ACTION_MOVE->{drag?.let{updateControl(it,e.x,e.y,true);invalidate()};return true}
            MotionEvent.ACTION_UP,MotionEvent.ACTION_CANCEL->{drag?.let{updateControl(it,e.x,e.y,true);saveSynthParams()};drag=null;parent?.requestDisallowInterceptTouchEvent(false);invalidate();performClick();return true}
        }
        return true
    }
    override fun performClick():Boolean{super.performClick();return true}

    private fun layoutValues():FloatArray {val w=width.toFloat();val h=height.toFloat();val gap=w*.014f;val y0=h*.018f+h*.092f;val sh=h*.176f;val y1=y0+sh+gap;val dy=y1+sh+gap;val dh=h*.215f;val my=dy+dh+gap;val mh=h*.176f;val fy=my+mh+gap;return floatArrayOf(w,h,w*.025f,gap,y0,sh,y1,dy,dh,my,mh,fy)}
    private fun findControl(x:Float,y:Float):Control? {
        val q=layoutValues();val w=q[0];val m=q[2];val ww=w-2*m;val sh=q[5]
        for(si in 0..1){val top=if(si==0)q[4]else q[6];val ky=top+sh*.49f;val kr=sh*.18f
            if(stepEdit[si])continue
            if(advancedEdit[si]){for(k in 6..7){val kx=m+ww*(if(k==6).34f else .66f);if(hypot((x-kx).toDouble(),(y-ky).toDouble())<=kr)return Control.SynthKnob(si,k)}}
            else for(k in 0..5){val kx=m+ww*(.075f+k*.145f);if(hypot((x-kx).toDouble(),(y-ky).toDouble())<=kr)return Control.SynthKnob(si,k)}}
        val mt=q[9];val mh=q[10]
        for(ch in 0..3){val cx=m+ww*(.18f+ch*.205f);val ky=mt+mh*.34f;val kr=mh*.15f;if(hypot((x-(cx-ww*.037f)).toDouble(),(y-ky).toDouble())<=kr)return Control.MixerKnob(ch,0);if(hypot((x-(cx+ww*.050f)).toDouble(),(y-ky).toDouble())<=kr)return Control.MixerKnob(ch,1);if(abs(x-cx)<=ww*.035f&&y in mt+mh*.45f..mt+mh*.93f)return Control.Fader(ch)}
        return null
    }
    private fun controlValue(c:Control)=when(c){is Control.SynthKnob->with(engine.synth[c.engine]){when(c.knob){6->slideTime;7->accentDecay;else->if(extendedEnvelope)when(c.knob){0->attack;1->decay;2->sustain;3->release;4->envMod;else->accentAmount}else when(c.knob){0->tune;1->cutoff;2->resonance;3->envMod;4->decay;else->accentAmount}}};is Control.MixerKnob->if(c.knob==0)engine.mixer[c.channel].pan else engine.mixer[c.channel].send;is Control.Fader->engine.mixer[c.channel].level}
    private fun updateControl(c:Control,x:Float,y:Float,relative:Boolean){
        val value=if(c is Control.Fader){val q=layoutValues();val top=q[9]+q[10]*.50f;val bottom=q[9]+q[10]*.88f;((bottom-y)/(bottom-top)).coerceIn(0f,1f)}else if(relative)(dragStartValue+(dragStartY-y)/(height*.18f)).coerceIn(0f,1f)else controlValue(c)
        when(c){is Control.SynthKnob->{val p=engine.synth[c.engine];when(c.knob){6->p.slideTime=value;7->p.accentDecay=value;else->if(p.extendedEnvelope)when(c.knob){0->p.attack=value;1->p.decay=value;2->p.sustain=value;3->p.release=value;4->p.envMod=value;else->p.accentAmount=value}else when(c.knob){0->p.tune=value;1->p.cutoff=value;2->p.resonance=value;3->p.envMod=value;4->p.decay=value;else->p.accentAmount=value}}};is Control.MixerKnob->{if(c.knob==0)engine.mixer[c.channel].pan=value else engine.mixer[c.channel].send=value};is Control.Fader->engine.mixer[c.channel].level=value}
    }
    private fun handleTap(x:Float,y:Float){
        val q=layoutValues();val w=q[0];val h=q[1];val m=q[2];val ww=w-2*m
        if(y<h*.10f&&x>w*.80f){togglePlay();return}
        if(y<h*.10f&&x in m+ww*.44f..m+ww*.62f){onProject?.invoke();return}
        for(si in 0..1){val top=if(si==0)q[4]else q[6];val sh=q[5];if(y !in top..top+sh)continue;selectedTrack=si;if(y in top+sh*.03f..top+sh*.29f){when{x in m+ww*.255f..m+ww*.365f->{engine.synth[si].squareWave=!engine.synth[si].squareWave;saveSynthParams();return};x in m+ww*.38f..m+ww*.49f->{engine.synth[si].extendedEnvelope=!engine.synth[si].extendedEnvelope;saveSynthParams();return};x in m+ww*.505f..m+ww*.65f->{stepEdit[si]=!stepEdit[si];accentEdit[si]=false;advancedEdit[si]=false;return};x in m+ww*.665f..m+ww*.80f->{accentEdit[si]=!accentEdit[si];stepEdit[si]=false;advancedEdit[si]=false;return};x in m+ww*.815f..m+ww*.94f->{advancedEdit[si]=!advancedEdit[si];stepEdit[si]=false;accentEdit[si]=false;return}}};if(stepEdit[si]&&y in top+sh*.29f..top+sh*.70f){handleStepEditorTap(si,x,y,m,top,ww,sh);return};if(y in top+sh*.70f..top+sh*.98f){val sg=ww*.008f;val sw=(ww*.91f-sg*15)/16f;val sx=m+ww*.045f;val i=((x-sx)/(sw+sg)).toInt();if(i in 0..15&&x>=sx+i*(sw+sg)&&x<=sx+i*(sw+sg)+sw){selectedAcidStep[si]=i;val s=if(si==0)pattern.acidA[i]else pattern.acidB[i];if(accentEdit[si]){if(s.active)s.accent=!s.accent}else s.cycleVelocity()}};return}
        val dt=q[7];val dh=q[8];if(y in dt..dt+dh){if(y<dt+dh*.22f&&x<m+ww*.40f){selectedTrack=if(x<m+ww*.20f)2 else 3}else{selectedTrack=selectedTrack.coerceAtLeast(2);toggleDrum(x,y,w,h,dt,dh,m,ww)};return}
        val mt=q[9];val mh=q[10];if(y in mt..mt+mh){for(ch in 0..3){val cx=m+ww*(.18f+ch*.205f);if(x in cx+ww*.064f..cx+ww*.124f){if(y in mt+mh*.47f..mt+mh*.63f){engine.mixer[ch].mute=!engine.mixer[ch].mute;return};if(y in mt+mh*.64f..mt+mh*.80f){engine.mixer[ch].solo=!engine.mixer[ch].solo;return}}};return}
        val ft=q[11];if(y>ft){val footerH=h-h*.018f-ft;if(y in ft+footerH*.28f..ft+footerH*.64f){val g=ww*.014f;val bw=(ww*.91f-g*7)/8;val sx=m+ww*.045f;val i=((x-sx)/(bw+g)).toInt();if(i in 0..7&&i!=bank){patternBanks.switchTo(i,pattern);bank=i}}else if(y>ft+footerH*.66f){when{x<m+ww*.31f->pattern.clear();x>m+ww*.61f->togglePlay();else->savePattern()}}}
    }
    private fun handleStepEditorTap(engineIndex:Int,x:Float,y:Float,l:Float,t:Float,ww:Float,hh:Float){
        val step=if(engineIndex==0)pattern.acidA[selectedAcidStep[engineIndex]]else pattern.acidB[selectedAcidStep[engineIndex]];val gap=ww*.008f;val sx=l+ww*.045f
        if(y in t+hh*.29f..t+hh*.52f){val bw=(ww*.72f-gap*6)/7f;val key=((x-sx)/(bw+gap)).toInt();if(key in 0..6)AcidStepEditor.setPitch(step,key);return}
        if(y in t+hh*.52f..t+hh*.72f){val cw=ww*.18f;when(((x-sx)/(cw+gap)).toInt()){0->AcidStepEditor.transpose(step,-12);1->AcidStepEditor.transpose(step,12);2->AcidStepEditor.toggleAccent(step);3->AcidStepEditor.toggleSlide(step)}}
    }
    private fun togglePlay(){playing=!playing;if(playing)engine.start()else engine.stop()}
    private fun toggleDrum(x:Float,y:Float,w:Float,h:Float,top:Float,sectionH:Float,m:Float,ww:Float){val sx=m+ww*.10f;val sg=ww*.006f;val sw=(ww*.865f-sg*15)/16f;val i=((x-sx)/(sw+sg)).toInt();if(i !in 0..15)return;val row=(((y-(top+sectionH*.27f))/(sectionH*.17f)).toInt()).coerceIn(0,3);val d=if(selectedTrack==3)pattern.drum9 else pattern.drum8;when(row){0->d.kick[i]=!d.kick[i];1->d.snare[i]=!d.snare[i];2->d.hat[i]=!d.hat[i];else->d.clap[i]=!d.clap[i]}}
    private fun savePattern(){patternBanks.store(pattern);context.getSharedPreferences("patterns",Context.MODE_PRIVATE).edit().putString("banks",patternBanks.encode()).apply();saveSynthParams()}
    private fun saveSynthParams(){
        val e=context.getSharedPreferences("synth",Context.MODE_PRIVATE).edit()
        engine.synth.forEachIndexed{i,p->e.putFloat("slideTime$i",p.slideTime).putFloat("accentDecay$i",p.accentDecay).putBoolean("square$i",p.squareWave).putBoolean("adsr$i",p.extendedEnvelope)}
        e.apply()
    }
    private fun loadSynthParams(){
        val p=context.getSharedPreferences("synth",Context.MODE_PRIVATE)
        engine.synth.forEachIndexed{i,s->s.slideTime=p.getFloat("slideTime$i",s.slideTime);s.accentDecay=p.getFloat("accentDecay$i",s.accentDecay);s.squareWave=p.getBoolean("square$i",s.squareWave);s.extendedEnvelope=p.getBoolean("adsr$i",s.extendedEnvelope)}
    }
    fun exportProjectJson():String{patternBanks.store(pattern);return ProjectStore.encode(engine.bpm,bank,patternBanks,engine.synth,engine.mixer)}
    fun importProjectJson(json:String):Boolean{
        val p=ProjectStore.decode(json)?:return false
        if(!patternBanks.restore(p.banksEncoded))return false
        patternBanks.switchTo(p.bank,pattern);bank=p.bank;engine.bpm=p.bpm
        for(i in 0..1){val s=engine.synth[i];val q=p.synth[i];s.tune=q.tune;s.cutoff=q.cutoff;s.resonance=q.resonance;s.envMod=q.envMod;s.decay=q.decay;s.accentAmount=q.accentAmount;s.squareWave=q.squareWave;s.extendedEnvelope=q.extendedEnvelope;s.attack=q.attack;s.sustain=q.sustain;s.release=q.release;s.slideTime=q.slideTime;s.accentDecay=q.accentDecay}
        for(i in 0..3){val m=engine.mixer[i];val q=p.mixer[i];m.pan=q.pan;m.send=q.send;m.level=q.level}
        saveSynthParams();invalidate();return true
    }
    fun exportStrudelCode()=StrudelBridge.exportCode(pattern,engine.bpm,engine.synth[0].squareWave,engine.synth[1].squareWave)
    fun importStrudel(text:String):StrudelBridge.ImportResult{val r=StrudelBridge.importCode(text);r.pattern?.let{pattern.copyFrom(it)};r.bpm?.let{engine.bpm=it};invalidate();return r}
    private fun serialize()=buildString{append("A:");pattern.acidA.forEach{append(if(it.active)it.velocity else 0);append(if(it.accent)'a' else '-')} ;append(";B:");pattern.acidB.forEach{append(if(it.active)it.velocity else 0);append(if(it.accent)'a' else '-')}}
}
