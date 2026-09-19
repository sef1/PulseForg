package com.sefi.pulseforge
import android.graphics.Bitmap
import android.graphics.Canvas
import android.view.MotionEvent
import androidx.test.core.app.ApplicationProvider
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.FileOutputStream
@RunWith(RobolectricTestRunner::class) @Config(sdk=[34]) @GraphicsMode(GraphicsMode.Mode.NATIVE)
class VisualRenderTest {
 @Test fun renderPhoneUiToPng(){val v=GrooveboxView(ApplicationProvider.getApplicationContext());v.measure(android.view.View.MeasureSpec.makeMeasureSpec(1080,1073741824),android.view.View.MeasureSpec.makeMeasureSpec(1920,1073741824));v.layout(0,0,1080,1920);fun tap(x:Float,y:Float){v.dispatchTouchEvent(MotionEvent.obtain(0,0,0,x,y,0));v.dispatchTouchEvent(MotionEvent.obtain(0,1,1,x,y,0))};tap(185f,485f);tap(346f,485f);tap(950f,235f);tap(308f,1533f);tap(519f,1587f);tap(620f,628f);val b=Bitmap.createBitmap(1080,1920,Bitmap.Config.ARGB_8888);v.draw(Canvas(b));FileOutputStream("/downloads/PulseForge-0.6.1-ui.png").use{b.compress(Bitmap.CompressFormat.PNG,100,it)}}
}
