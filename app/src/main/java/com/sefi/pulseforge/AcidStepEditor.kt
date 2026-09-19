package com.sefi.pulseforge

/** Editing operations shared by the touch panel and tests. */
object AcidStepEditor {
    private val pitchClasses=intArrayOf(0,2,4,5,7,9,11)
    fun setPitch(step:Step,key:Int){require(key in 0..6);val octave=Math.floorDiv(step.note,12);step.note=(octave*12+pitchClasses[key]).coerceIn(12,96);step.active=true}
    fun transpose(step:Step,semitones:Int){step.note=(step.note+semitones).coerceIn(12,96);step.active=true}
    fun toggleAccent(step:Step){step.active=true;step.accent=!step.accent}
    fun toggleSlide(step:Step){step.active=true;step.slide=!step.slide}
}
