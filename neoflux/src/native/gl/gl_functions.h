// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/gl/gl_functions.h
//
// Desktop/ES GL entry-point loader and RAII wrappers for the handful of GL
// objects NeoFlux drives directly (framebuffers and textures).
//
// Why this exists: libmpv's render_gl API asks us to supply a get_proc_address
// callback and then render decoded video into an FBO we own. Rather than leak
// raw glGen*/glDelete* calls and glfwGetProcAddress into the media layer, all
// GL plumbing lives here under neoflux::gl. Future native backends
// (native/vulkan, native/d3d12, native/metal) follow the same pattern: a
// per-backend module that owns its function pointers and RAII handles.
//
// Threading / ownership contract (the caller, mpv_media_player.cpp, relies on
// this exactly):
//   - GlTexture / GlFramebuffer wrap a single GLuint handle (0 = empty).
//   - They are move-only. Copying is deleted.
//   - Release() explicitly calls glDelete* on the handle and resets it to 0.
//     It is idempotent and a no-op when the handle is 0 or the loader could
//     not resolve the delete entry point.
//   - The destructor calls Release(). This means:
//       * Production: TeardownRender() runs Release() on the render thread (the
//         thread whose GL context owns the object); a later destructor on the
//         App thread sees handle == 0 and does nothing (the App thread has no
//         current GL context).
//       * Unit tests: the test owns a GL context on the calling thread, so the
//         destructor runs glDelete* inline on that same thread.
//     The wrappers MUST NOT call GL in the destructor unconditionally; the
//     explicit Release() is what keeps object destruction off the wrong thread.
// =============================================================================

#ifndef NEOFLUX_NATIVE_GL_FUNCTIONS_H_
#define NEOFLUX_NATIVE_GL_FUNCTIONS_H_

#include <cstdint>

#ifdef _WIN32
// APIENTRY marks the Win32 __stdcall calling convention used by GL function
// pointers on Windows. windows.h defines it; GLFW already pulls it in on Win32
// desktop builds, so this adds no real overhead there.
#include <windows.h>
#endif

// Off Windows, APIENTRY is undefined but GL function pointers are cdecl, so an
// empty macro is correct. Define it here so the GlApi typedefs compile on Linux.
#ifndef APIENTRY
#define APIENTRY
#endif

namespace neoflux {
namespace gl {

// GL scalar aliases kept deliberately narrow so the pointer signatures read
// like the real GL headers without pulling in gl.h (which would clash with our
// own custom loader on some toolchains).
using GlEnum = unsigned int;
using GlUint = unsigned int;
using GlInt = int;
using GlSizei = int;

// Set of GL 2.1 / FBO extension entry points NeoFlux drives directly. All
// pointers are null until Load() runs on a thread with a current GL context.
struct GlApi {
  void (APIENTRY* GenFramebuffers)(GlSizei, GlUint*) = nullptr;
  void (APIENTRY* DeleteFramebuffers)(GlSizei, const GlUint*) = nullptr;
  void (APIENTRY* BindFramebuffer)(GlEnum, GlUint) = nullptr;
  void (APIENTRY* FramebufferTexture2D)(GlEnum, GlEnum, GlEnum, GlUint, GlInt) = nullptr;
  void (APIENTRY* GenTextures)(GlSizei, GlUint*) = nullptr;
  void (APIENTRY* DeleteTextures)(GlSizei, const GlUint*) = nullptr;
  void (APIENTRY* BindTexture)(GlEnum, GlUint) = nullptr;
  void (APIENTRY* TexImage2D)(GlEnum, GlInt, GlInt, GlSizei, GlSizei, GlInt, GlEnum,
                              GlEnum, const void*) = nullptr;
  void (APIENTRY* TexParameteri)(GlEnum, GlEnum, GlInt) = nullptr;
};

// Process-wide GL entry-point table. Load() resolves every pointer through the
// platform's proc-address mechanism (glfwGetProcAddress on desktop) and is
// safe to call repeatedly. On non-desktop platforms Load() is a no-op and every
// pointer stays null; a platform native implementation fills this in later.
GlApi& GetGlApi();
void Load();

// Platform proc-address trampoline. On desktop this forwards to
// glfwGetProcAddress; on other platforms it returns nullptr. The media layer
// uses this as mpv's render_gl get_proc_address callback WITHOUT referencing
// GLFW directly.
void* GetProcAddress(const char* name);

// ---------------------------------------------------------------------------
// RAII move-only wrappers.
// ---------------------------------------------------------------------------

// Owns a GL texture object. Create() generates the texture and applies the
// fixed sampler parameters NeoFlux needs (LINEAR filtering, CLAMP_TO_EDGE).
// EnsureStorage() (re)allocates RGBA8 storage only when the size changes.
class GlTexture {
 public:
  GlTexture() = default;
  ~GlTexture();

  // Move-only: the GLuint handle transfers, leaving the source empty.
  GlTexture(GlTexture&& other) noexcept;
  GlTexture& operator=(GlTexture&& other) noexcept;
  GlTexture(const GlTexture&) = delete;
  GlTexture& operator=(const GlTexture&) = delete;

  // Generates the texture object and sets MIN/MAG filter = LINEAR,
  // WRAP_S/WRAP_T = CLAMP_TO_EDGE. No-op if a handle is already owned.
  void Create();

  // Binds this texture to the GL_TEXTURE_2D target. No-op when empty.
  void Bind() const;

  // (Re)allocates RGBA8 storage of |w| x |h| only when the size differs from
  // the last allocation. Binds the texture internally. No-op when empty.
  void EnsureStorage(int w, int h);

  // Explicit glDeleteTextures and reset to 0. Idempotent; call on the thread
  // that owns the GL context (see file-header contract).
  void Release();

  GlUint id() const { return handle_; }
  bool valid() const { return handle_ != 0; }
  // Dimensions of the storage last allocated via EnsureStorage (0 = none).
  int allocated_width() const { return allocated_w_; }
  int allocated_height() const { return allocated_h_; }

 private:
  GlUint handle_ = 0;
  int allocated_w_ = 0;
  int allocated_h_ = 0;
};

// Owns a GL framebuffer object. Create() generates the FBO;
// AttachColorTexture() binds a texture as COLOR_ATTACHMENT0; Unbind() rebinds
// the default framebuffer.
class GlFramebuffer {
 public:
  GlFramebuffer() = default;
  ~GlFramebuffer();

  GlFramebuffer(GlFramebuffer&& other) noexcept;
  GlFramebuffer& operator=(GlFramebuffer&& other) noexcept;
  GlFramebuffer(const GlFramebuffer&) = delete;
  GlFramebuffer& operator=(const GlFramebuffer&) = delete;

  // Generates the FBO object. No-op if a handle is already owned.
  void Create();

  // Binds this FBO to the GL_FRAMEBUFFER target. No-op when empty.
  void Bind() const;

  // Rebinds the default framebuffer (target 0).
  static void Unbind();

  // Attaches texture |tex_id| as COLOR_ATTACHMENT0. Binds this FBO internally.
  void AttachColorTexture(GlUint tex_id) const;

  // Explicit glDeleteFramebuffers and reset to 0. Idempotent.
  void Release();

  GlUint id() const { return handle_; }
  bool valid() const { return handle_ != 0; }

 private:
  GlUint handle_ = 0;
};

}  // namespace gl
}  // namespace neoflux

#endif  // NEOFLUX_NATIVE_GL_FUNCTIONS_H_
