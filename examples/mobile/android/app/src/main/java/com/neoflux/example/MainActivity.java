// =============================================================================
// NeoFlux Android Example - MainActivity.java
//
// Demonstrates how to embed a NeoFlux rendering surface in an Android Activity.
// The native code (native-lib.cpp) creates the Application and widget tree,
// while this Activity provides the SurfaceView for rendering.
// =============================================================================

package com.neoflux.example;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

/**
 * Main activity for the NeoFlux Android example.
 *
 * <p>Sets up a full-screen SurfaceView and passes the Surface to native code
 * for NeoFlux rendering. The native code handles the widget tree, layout, and
 * rendering; this Activity only manages the Android lifecycle.
 */
public class MainActivity extends Activity implements SurfaceHolder.Callback {

  static {
    // Load the native library containing NeoFlux and the example widget tree.
    System.loadLibrary("neoflux_example");
  }

  private SurfaceView surfaceView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Keep the screen on during rendering.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    // Create a full-screen SurfaceView for NeoFlux rendering.
    surfaceView = new SurfaceView(this);
    surfaceView.getHolder().addCallback(this);
    setContentView(surfaceView);
  }

  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    // Surface is ready; pass it to native code to start rendering.
    nativeOnSurfaceCreated(holder.getSurface());
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width,
                             int height) {
    // Surface size changed; notify native code to resize the renderer.
    nativeOnSurfaceChanged(width, height);
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    // Surface is being destroyed; stop rendering in native code.
    nativeOnSurfaceDestroyed();
  }

  // Native methods implemented in native-lib.cpp.
  private native void nativeOnSurfaceCreated(android.view.Surface surface);

  private native void nativeOnSurfaceChanged(int width, int height);

  private native void nativeOnSurfaceDestroyed();
}
