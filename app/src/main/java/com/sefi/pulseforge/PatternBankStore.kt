package com.sefi.pulseforge

/** Eight independent in-memory pattern slots. STORE commits the edit buffer. */
class PatternBankStore(initial: Pattern = Pattern.demo()) {
    private val slots = Array(8) { Pattern() }
    var currentBank: Int = 0
        private set

    init { slots[0] = initial.deepCopy() }

    fun store(editBuffer: Pattern) { slots[currentBank] = editBuffer.deepCopy() }
    fun switchTo(index: Int, editBuffer: Pattern) {
        require(index in 0..7)
        currentBank = index
        editBuffer.copyFrom(slots[index])
    }
    fun snapshot(index: Int) = slots[index.coerceIn(0,7)].deepCopy()

    fun encode(): String = slots.joinToString("|") { encodePattern(it) }
    fun restore(encoded: String): Boolean {
        val values=encoded.split('|'); if(values.size!=8)return false
        return try { values.forEachIndexed { i,v -> slots[i]=decodePattern(v) }; true } catch(_:Exception){false}
    }

    private fun encodePattern(p:Pattern)=buildString {
        fun step(s:Step)=append(if(s.active)'1' else '0').append(',').append(s.note).append(',').append(if(s.accent)'1' else '0').append(',').append(s.velocity).append(',').append(if(s.slide)'1' else '0').append(';')
        p.acidA.forEach(::step); append('/'); p.acidB.forEach(::step); append('/')
        fun drum(d:DrumPattern){for(i in 0..15)append(if(d.kick[i])'1' else '0').append(if(d.snare[i])'1' else '0').append(if(d.hat[i])'1' else '0').append(if(d.clap[i])'1' else '0')}
        drum(p.drum8);append('/');drum(p.drum9)
    }
    private fun decodePattern(s:String):Pattern {
        val groups=s.split('/'); require(groups.size==4); val p=Pattern()
        fun steps(text:String,to:Array<Step>){val a=text.split(';').filter{it.isNotEmpty()};require(a.size==16);a.forEachIndexed{i,v->val f=v.split(',');require(f.size==5);to[i].apply{active=f[0]=="1";note=f[1].toInt();accent=f[2]=="1";velocity=f[3].toInt();slide=f[4]=="1"}}}
        steps(groups[0],p.acidA);steps(groups[1],p.acidB)
        fun drum(text:String,d:DrumPattern){require(text.length==64);for(i in 0..15){d.kick[i]=text[i*4]=='1';d.snare[i]=text[i*4+1]=='1';d.hat[i]=text[i*4+2]=='1';d.clap[i]=text[i*4+3]=='1'}}
        drum(groups[2],p.drum8);drum(groups[3],p.drum9);return p
    }
}
