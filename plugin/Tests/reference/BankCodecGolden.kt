// Golden-vector generator: encodes the default PatternBankStore content with
// the real Kotlin codec. With a file argument, decodes that file with the real
// codec and re-encodes (the reverse direction of the interop check).
package com.sefi.pulseforge

fun main(args: Array<String>) {
    val store = PatternBankStore()
    if (args.isNotEmpty()) {
        val s = java.io.File(args[0]).readText().trim()
        check(store.restore(s)) { "Kotlin restore rejected the C++ encoding" }
    }
    print(store.encode())
}
