// =============================================================================
// NeoFlux Android Example - native-lib.cpp
//
// JNI bridge between MainActivity.java and the NeoFlux C++ framework.
// Creates an Application with a simple widget tree (Text + Button) and renders
// to the Surface provided by the Android Activity.
// =============================================================================

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <memory>

#include "neoflux/application.h"
#include "neoflux/widget/text.h"
#include "neoflux/widget/button.h"
#include "neoflux/widget/column.h"

namespace {

// Global Application instance. Kept as a raw pointer because JNI callbacks
// need stable access; the Application owns all widgets via RAII internally.
neoflux::Application* g_app = nullptr;
ANativeWindow* g_window = nullptr;

}  // namespace

extern "C" {

/**
 * Called when the Surface is created. Initializes the NeoFlux Application
 * with the ANativeWindow as the rendering target.
 */
JNIEXPORT void JNICALL
Java_com_neoflux_example_MainActivity_nativeOnSurfaceCreated(
    JNIEnv* env, jobject /* thiz */, jobject surface) {
  if (g_app != nullptr) {
    return;  // Already initialized.
  }

  g_window = ANativeWindow_fromSurface(env, surface);
  if (g_window == nullptr) {
    return;
  }

  // Create the Application with the native window as the rendering surface.
  g_app = new neoflux::Application();
  g_app->SetFontDir("fonts");
  g_app->Init(ANativeWindow_getWidth(g_window),
              ANativeWindow_getHeight(g_window),
              reinterpret_cast<void*>(g_window));

  // Build a simple widget tree: Column with Text and Button.
  auto root = std::make_shared<neoflux::Column>();
  root->AddChild(std::make_shared<neoflux::Text>("Hello NeoFlux on Android!"));

  auto button = std::make_shared<neoflux::Button>("Click Me");
  button->SetOnClick([]() {
    // Button click handler.
  });
  root->AddChild(button);

  g_app->SetRoot(root);
}

/**
 * Called when the Surface size changes. Resizes the renderer.
 */
JNIEXPORT void JNICALL
Java_com_neoflux_example_MainActivity_nativeOnSurfaceChanged(
    JNIEnv* /* env */, jobject /* thiz */, jint width, jint height) {
  if (g_app != nullptr) {
    g_app->Resize(width, height);
  }
}

/**
 * Called when the Surface is destroyed. Stops rendering and cleans up.
 */
JNIEXPORT void JNICALL
Java_com_neoflux_example_MainActivity_nativeOnSurfaceDestroyed(
    JNIEnv* /* env */, jobject /* thiz */) {
  if (g_app != nullptr) {
    delete g_app;
    g_app = nullptr;
  }
  if (g_window != nullptr) {
    ANativeWindow_release(g_window);
    g_window = nullptr;
  }
}

}  // extern "C"
