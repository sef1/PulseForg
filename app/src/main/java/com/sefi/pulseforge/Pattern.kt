package com.sefi.pulseforge

data class Step(
    var active: Boolean = false,
    var note: Int = 36,
    var accent: Boolean = false,
    var velocity: Int = 1,
    var slide: Boolean = false
) {
    fun cycleVelocity() {
        when {
            !active -> { active = true; velocity = 1 }
            velocity == 1 -> velocity = 2
            else -> { active = false; velocity = 1; accent = false }
        }
    }
}

data class DrumPattern(
    val kick: BooleanArray = BooleanArray(16),
    val snare: BooleanArray = BooleanArray(16),
    val hat: BooleanArray = BooleanArray(16),
    val clap: BooleanArray = BooleanArray(16)
) {
    fun clear() { kick.fill(false); snare.fill(false); hat.fill(false); clap.fill(false) }
}

data class Pattern(
    val acidA: Array<Step> = Array(16) { Step(note = 36 + (it % 5)) },
    val acidB: Array<Step> = Array(16) { Step(note = 29 + (it % 7)) },
    val drum8: DrumPattern = DrumPattern(),
    val drum9: DrumPattern = DrumPattern()
) {
    fun clear() {
        (acidA + acidB).forEach { it.active = false; it.accent = false; it.velocity = 1; it.slide = false }
        drum8.clear(); drum9.clear()
    }

    companion object {
        fun demo() = Pattern().apply {
            intArrayOf(0, 3, 6, 8, 11, 14).forEach { acidA[it].active = true }
            acidA[3].velocity = 2; acidA[3].accent = true; acidA[11].slide = true
            intArrayOf(2, 6, 10, 14).forEach { acidB[it].active = true }
            acidB[6].velocity = 2; acidB[14].accent = true
            intArrayOf(0, 4, 8, 12).forEach { drum8.kick[it] = true }
            intArrayOf(4, 12).forEach { drum8.snare[it] = true; drum8.clap[it] = true }
            for (i in 0 until 16 step 2) drum8.hat[i] = true
            intArrayOf(0, 3, 7, 10, 12).forEach { drum9.kick[it] = true }
            intArrayOf(4, 12).forEach { drum9.snare[it] = true }
            for (i in 1 until 16 step 2) drum9.hat[i] = true
            intArrayOf(6, 14).forEach { drum9.clap[it] = true }
        }
    }
}

fun Pattern.copyFrom(other: Pattern) {
    for (i in 0..15) {
        acidA[i].apply { active=other.acidA[i].active; note=other.acidA[i].note; accent=other.acidA[i].accent; velocity=other.acidA[i].velocity; slide=other.acidA[i].slide }
        acidB[i].apply { active=other.acidB[i].active; note=other.acidB[i].note; accent=other.acidB[i].accent; velocity=other.acidB[i].velocity; slide=other.acidB[i].slide }
    }
    fun copyDrum(to: DrumPattern, from: DrumPattern) { for (i in 0..15) { to.kick[i]=from.kick[i]; to.snare[i]=from.snare[i]; to.hat[i]=from.hat[i]; to.clap[i]=from.clap[i] } }
    copyDrum(drum8,other.drum8); copyDrum(drum9,other.drum9)
}

fun Pattern.deepCopy() = Pattern().also { it.copyFrom(this) }
