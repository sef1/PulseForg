package com.sefi.pulseforge
import org.junit.Assert.*
import org.junit.Test
class AcidVoiceModelTest {
 @Test fun slideRoutesPitchContinuouslyTowardNextNote(){val start=AcidVoiceModel.glideFrequency(110.0,220.0,0.0,true);val middle=AcidVoiceModel.glideFrequency(110.0,220.0,.03,true);val end=AcidVoiceModel.glideFrequency(110.0,220.0,.25,true);assertEquals(110.0,start,1e-9);assertTrue(middle in 110.0..220.0);assertTrue(end>middle);assertEquals(220.0,AcidVoiceModel.glideFrequency(110.0,220.0,0.0,false),1e-9)}
 @Test fun waveformToggleRoutesSawAndSquare(){assertEquals(-.5,AcidVoiceModel.oscillator(.25,false),1e-9);assertEquals(1.0,AcidVoiceModel.oscillator(.25,true),1e-9);assertEquals(-1.0,AcidVoiceModel.oscillator(.75,true),1e-9)}
 @Test fun classicDefaultRetainsDecayEnvelope(){val fast=AcidVoiceModel.amplitudeEnvelope(.1,.12,false,1.0,0.0,1.0,1.0);val slow=AcidVoiceModel.amplitudeEnvelope(.1,.12,false,0.0,1.0,0.0,0.0);assertTrue(slow>fast)}
 @Test fun expandedAdsrRoutesAttackSustainAndRelease(){val earlySlow=AcidVoiceModel.amplitudeEnvelope(.01,.4,true,1.0,.5,.8,.8);val earlyFast=AcidVoiceModel.amplitudeEnvelope(.01,.4,true,0.0,.5,.8,.8);assertTrue(earlyFast>earlySlow);val highS=AcidVoiceModel.amplitudeEnvelope(.25,.4,true,.1,.2,1.0,.5);val lowS=AcidVoiceModel.amplitudeEnvelope(.25,.4,true,.1,.2,0.0,.5);assertTrue(highS>lowS);val shortR=AcidVoiceModel.amplitudeEnvelope(.39,.4,true,.1,.2,.6,0.0);val longR=AcidVoiceModel.amplitudeEnvelope(.39,.4,true,.1,.2,.6,1.0);assertTrue(longR>shortR)}
 @Test fun slideTimeChangesGlideDuration(){
  val fast=AcidVoiceModel.glideFrequency(110.0,220.0,.08,true,0.0)
  val slow=AcidVoiceModel.glideFrequency(110.0,220.0,.08,true,1.0)
  assertEquals(220.0,fast,1e-9);assertTrue(slow<200.0);assertTrue(slow>110.0)
 }
}
