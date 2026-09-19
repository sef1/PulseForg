package com.sefi.pulseforge

import android.app.Activity
import android.app.AlertDialog
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.Window
import android.widget.EditText
import android.widget.Toast

class MainActivity : Activity() {
    private lateinit var surface: GrooveboxView
    private val createDoc = 41
    private val openDoc = 42

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.statusBarColor = 0xff101410.toInt()
        window.navigationBarColor = 0xff101410.toInt()
        surface = GrooveboxView(this)
        surface.onProject = { showProjectMenu() }
        setContentView(surface)
    }
    override fun onPause() { super.onPause(); surface.stopAudio() }

    private fun showProjectMenu() {
        val items = arrayOf("Save project to file", "Load project from file", "Copy Strudel export", "Open in strudel.cc", "Import from Strudel")
        AlertDialog.Builder(this).setTitle("PROJECT").setItems(items) { _, which ->
            when (which) {
                0 -> startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("application/json").putExtra(Intent.EXTRA_TITLE, "pulseforge-project.json"), createDoc)
                1 -> startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*"), openDoc)
                2 -> { clipboard(surface.exportStrudelCode()); toast("Strudel code copied. Accents, slides, velocity and filter settings are not converted.") }
                3 -> startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(StrudelBridge.exportUrl(surface.exportStrudelCode()))))
                4 -> showStrudelImport()
            }
        }.show()
    }

    private fun showStrudelImport() {
        val input = EditText(this).apply { minLines = 4; hint = "Paste Strudel code or a strudel.cc link" }
        AlertDialog.Builder(this).setTitle("IMPORT FROM STRUDEL").setView(input)
            .setPositiveButton("IMPORT") { _, _ ->
                val r = surface.importStrudel(input.text.toString())
                if (r.pattern == null) toast("No supported note or sound rows found")
                else toast("Imported: " + r.applied.joinToString(", ") + if (r.skipped.isNotEmpty()) " | Skipped ${r.skipped.size} unsupported item(s)" else "")
            }
            .setNegativeButton("CANCEL", null).show()
    }

    private fun clipboard(text: String) { (getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager).setPrimaryClip(ClipData.newPlainText("PulseForge Strudel export", text)) }
    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_LONG).show()

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        when (requestCode) {
            createDoc -> try {
                contentResolver.openOutputStream(uri)?.use { it.write(surface.exportProjectJson().toByteArray()) }
                toast("Project saved")
            } catch (e: Exception) { toast("Save failed: ${e.message}") }
            openDoc -> try {
                val json = contentResolver.openInputStream(uri)?.readBytes()?.toString(Charsets.UTF_8) ?: throw IllegalStateException("empty file")
                toast(if (surface.importProjectJson(json)) "Project loaded" else "Not a PulseForge project file")
            } catch (e: Exception) { toast("Load failed: ${e.message}") }
        }
    }
}
