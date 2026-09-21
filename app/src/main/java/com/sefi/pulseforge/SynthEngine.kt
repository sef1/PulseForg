package com.sefi.pulseforge

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import kotlin.concurrent.thread
import kotlin.math.PI
import kotlin.math.exp
import kotlin.math.pow
import kotlin.math.sin
import kotlin.math.tanh
import kotlin.random.Random

class SynthParams {
    @Volatile var tune = .5f
    @Volatile var cutoff = .58f
    @Volatile var resonance = .42f
    @Volatile var envMod = .62f
    @Volatile var decay = .54f
    @Volatile var accentAmount = .68f
    @Volatile var squareWave = false
    @Volatile var extendedEnvelope = false
    @Volatile var attack = .08f
    @Volatile var sustain = .62f
    @Volatile var release = .18f
    @Volatile var slideTime = .30f
    @Volatile var accentDecay = .35f
}

class MixerChannel {
    @Volatile var pan = .5f
    @Volatile var send = .25f
    @Volatile var level = .75f
    @Volatile var mute = false
    @Volatile var solo = false
}

class SynthEngine(private val pattern: Pattern, private val onStep: (Int) -> Unit) {
    @Volatile var bpm = 126f
    @Volatile var drive = .25f
    @Volatile var playing = false
    val synth = arrayOf(SynthParams(), SynthParams().apply { tune=.55f; cutoff=.58f; resonance=.46f; envMod=.72f; decay=.61f; accentAmount=.52f })
    val mixer = arrayOf(
        MixerChannel().apply { pan=.42f; send=.30f; level=.78f },
        MixerChannel().apply { pan=.58f; send=.26f; level=.70f },
        MixerChannel().apply { pan=.48f; send=.18f; level=.82f },
        MixerChannel().apply { pan=.52f; send=.20f; level=.72f }
    )
    // Backward-compatible aliases used by earlier tests/source clients.
    var cutoff: Float get() = synth[0].cutoff; set(v) { synth[0].cutoff = v.coerceIn(0f,1f) }
    var resonance: Float get() = synth[0].resonance; set(v) { synth[0].resonance = v.coerceIn(0f,1f) }

    /** Mute always silences its channel. When any SOLO is active, only soloed (unmuted) channels are audible. */
    fun channelAudible(ch: Int): Boolean {
        val anySolo = mixer.any { it.solo }
        val mc = mixer[ch]
        return if (anySolo) mc.solo && !mc.mute else !mc.mute
    }

    private var worker: Thread? = null
    private val sampleRate = 44100

    fun start() { if (!playing) { playing = true; worker = thread(name = "PulseForgeAudio") { runAudio() } } }
    fun stop() { playing = false; worker?.join(300); worker = null; onStep(-1) }

    private fun runAudio() {
        val track = AudioTrack.Builder()
            .setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
            .setAudioFormat(AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
            .setTransferMode(AudioTrack.MODE_STREAM).setBufferSizeInBytes(8192).build()
        track.play()
        var phaseA=0.0; var phaseB=0.0; var step=0
        val filterA=AcidResonantFilter(sampleRate.toDouble()); val filterB=AcidResonantFilter(sampleRate.toDouble())
        var delayL=0.0; var delayR=0.0
        val accentA=AcidAccentModel(sampleRate); val accentB=AcidAccentModel(sampleRate)
        while (playing) {
            val frames=(sampleRate*60f/bpm/4f).toInt().coerceAtLeast(1000)
            val pcm=ShortArray(frames*2); val a=pattern.acidA[step]; val b=pattern.acidB[step]
            val previous=(step+15)%16; val previousA=pattern.acidA[previous]; val previousB=pattern.acidB[previous]
            val pa=synth[0]; val pb=synth[1]; val d8=pattern.drum8; val d9=pattern.drum9
            val fa=midi(a.note + ((pa.tune-.5f)*24).toInt()); val fb=midi(b.note + ((pb.tune-.5f)*24).toInt())
            val fromFa=midi(previousA.note + ((pa.tune-.5f)*24).toInt()); val fromFb=midi(previousB.note + ((pb.tune-.5f)*24).toInt())
            val slideA=previousA.active&&previousA.slide&&a.active; val slideB=previousB.active&&previousB.slide&&b.active
            accentA.beginStep(a.active && a.accent, pa.accentAmount.toDouble())
            accentB.beginStep(b.active && b.accent, pb.accentAmount.toDouble())
            for(i in 0 until frames) {
                val t=i.toDouble()/sampleRate
                var acidA=0.0; var acidB=0.0; var drums8=0.0; var drums9=0.0
                if(a.active) {
                    val currentFa=AcidVoiceModel.glideFrequency(fromFa,fa,t,slideA,pa.slideTime.toDouble()); phaseA=(phaseA+currentFa/sampleRate)%1.0; val raw=AcidVoiceModel.oscillator(phaseA,pa.squareWave)
                    val accentFrame=accentA.sample(t,a.accent,pa.accentAmount.toDouble(),pa.resonance.toDouble(),pa.decay.toDouble(),pa.accentDecay.toDouble())
                    val ampEnv=AcidVoiceModel.amplitudeEnvelope(t,frames.toDouble()/sampleRate,pa.extendedEnvelope,pa.attack.toDouble(),pa.decay.toDouble(),pa.sustain.toDouble(),pa.release.toDouble())
                    val sweep=1.0+pa.envMod*accentFrame.filterEnvelope*1.8+accentFrame.filterSweep*.34
                    val filtered=filterA.process(raw,pa.cutoff.toDouble(),sweep,pa.resonance.toDouble())
                    val velocity=if(a.velocity==2)1.28 else 1.0
                    acidA=filtered*ampEnv*.31*velocity*(1.0+accentFrame.amplitudeLift)
                }
                if(b.active) {
                    val currentFb=AcidVoiceModel.glideFrequency(fromFb,fb,t,slideB,pb.slideTime.toDouble()); phaseB=(phaseB+currentFb/sampleRate)%1.0; val raw=AcidVoiceModel.oscillator(phaseB,pb.squareWave)
                    val accentFrame=accentB.sample(t,b.accent,pb.accentAmount.toDouble(),pb.resonance.toDouble(),pb.decay.toDouble(),pb.accentDecay.toDouble())
                    val ampEnv=AcidVoiceModel.amplitudeEnvelope(t,frames.toDouble()/sampleRate,pb.extendedEnvelope,pb.attack.toDouble(),pb.decay.toDouble(),pb.sustain.toDouble(),pb.release.toDouble())
                    val sweep=1.0+pb.envMod*accentFrame.filterEnvelope*1.8+accentFrame.filterSweep*.34
                    val filtered=filterB.process(raw,pb.cutoff.toDouble(),sweep,pb.resonance.toDouble())
                    val velocity=if(b.velocity==2)1.28 else 1.0
                    acidB=filtered*ampEnv*.28*velocity*(1.0+accentFrame.amplitudeLift)
                }
                if(d8.kick[step]) drums8+=sin(2*PI*(48.0+105.0*exp(-t*30))*t)*exp(-t*13)*.58
                if(d8.snare[step]) drums8+=(Random.nextDouble(-1.0,1.0)*.7+sin(2*PI*185*t)*.3)*exp(-t*22)*.24
                if(d8.hat[step]) drums8+=Random.nextDouble(-1.0,1.0)*sin(2*PI*6200*t)*exp(-t*65)*.10
                if(d8.clap[step]&&(i%337<85)) drums8+=Random.nextDouble(-1.0,1.0)*exp(-t*17)*.13
                if(d9.kick[step]) drums9+=(sin(2*PI*(56.0+155.0*exp(-t*38))*t)*exp(-t*19)+if(i<80)Random.nextDouble(-.2,.2)else 0.0)*.48
                if(d9.snare[step]) drums9+=(Random.nextDouble(-1.0,1.0)*.8+sin(2*PI*205*t)*.2)*exp(-t*27)*.22
                if(d9.hat[step]) drums9+=Random.nextDouble(-1.0,1.0)*sin(2*PI*7800*t)*exp(-t*78)*.09
                if(d9.clap[step]&&(i%271<65)) drums9+=Random.nextDouble(-1.0,1.0)*exp(-t*22)*.12
                val voices=doubleArrayOf(acidA,acidB,drums8,drums9)
                var left=0.0; var right=0.0; var sendL=0.0; var sendR=0.0
                for(ch in 0..3) {
                    if(!channelAudible(ch)) continue
                    val mc=mixer[ch]; val signal=voices[ch]*mc.level
                    val l=signal*(1.0-mc.pan*.78); val r=signal*(.22+mc.pan*.78)
                    left+=l; right+=r; sendL+=l*mc.send; sendR+=r*mc.send
                }
                delayL=delayL*.91+sendR*.09; delayR=delayR*.91+sendL*.09
                left+=delayL*.22; right+=delayR*.22
                left=tanh(left*(1.0+drive*3.0))*.78; right=tanh(right*(1.0+drive*3.0))*.78
                pcm[i*2]=(left.coerceIn(-1.0,1.0)*Short.MAX_VALUE).toInt().toShort()
                pcm[i*2+1]=(right.coerceIn(-1.0,1.0)*Short.MAX_VALUE).toInt().toShort()
            }
            onStep(step); track.write(pcm,0,pcm.size,AudioTrack.WRITE_BLOCKING); step=(step+1)%16
        }
        track.stop(); track.release()
    }
    private fun midi(note:Int)=440.0*2.0.pow((note-69)/12.0)
}
