// Golden-vector generator for the Strudel bridge: drives the real Kotlin
// StrudelBridge.kt. Modes:
//   export / exportWaves - print exportCode for the demo pattern at 126 bpm
//   url                  - print exportUrl of the plain export
//   import <file>        - importCode results in a canonical text form
package com.sefi.pulseforge

fun main(args: Array<String>) {
    when (args.getOrNull(0)) {
        "export" -> print(StrudelBridge.exportCode(Pattern.demo(), 126f, false, false))
        "exportWaves" -> print(StrudelBridge.exportCode(Pattern.demo(), 126f, true, true))
        "url" -> print(StrudelBridge.exportUrl(StrudelBridge.exportCode(Pattern.demo(), 126f, false, false)))
        "import" -> {
            val r = StrudelBridge.importCode(java.io.File(args[1]).readText())
            val sb = StringBuilder()
            sb.append("bpm=").append(r.bpm?.toString() ?: "null").append('\n')
            sb.append("applied=").append(r.applied.joinToString(",")).append('\n')
            sb.append("skipped=").append(r.skipped.joinToString(",")).append('\n')
            sb.append("banks=").append(r.pattern?.let { PatternBankStore(it).encode() } ?: "null")
            print(sb.toString())
        }
    }
}
