// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/gl/gl_functions.cpp
//
// Implementation of the GL entry-point loader and the RAII FBO/texture
// wrappers declared in gl_functions.h.
// =============================================================================

#include "native/gl/gl_functions.h"

#ifdef NEOFLUX_PLATFORM_DESKTOP
#include <GLFW/glfw3.h>
#endif

namespace neoflux {
namespace gl {
namespace {

// GL constants kept out of the header so no consumer leaks raw GL enum names;
// the wrappers encapsulate every call site. Values are the canonical GL 2.1 /
// FBO extension defines.
constexpr GlEnum kTexture2d = 0x0DE1;
constexpr GlEnum kTextureMinFilter = 0x2801;
constexpr GlEnum kTextureMagFilter = 0x2800;
constexpr GlEnum kTextureWrapS = 0x2802;
constexpr GlEnum kTextureWrapT = 0x2803;
constexpr GlInt kLinear = 0x2601;
constexpr GlInt kClampToEdge = 0x812F;
constexpr GlEnum kFramebuffer = 0x8D40;
constexpr GlInt kRgba = 0x1908;
constexpr GlEnum kUnsignedByte = 0x1401;
constexpr GlEnum kColorAttachment0 = 0x8CE0;

GlApi g_gl_api;

}  // namespace

GlApi& GetGlApi() { return g_gl_api; }

void Load() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  g_gl_api.GenFramebuffers = reinterpret_cast<decltype(g_gl_api.GenFramebuffers)>(
      glfwGetProcAddress("glGenFramebuffers"));
  g_gl_api.DeleteFramebuffers = reinterpret_cast<decltype(g_gl_api.DeleteFramebuffers)>(
      glfwGetProcAddress("glDeleteFramebuffers"));
  g_gl_api.BindFramebuffer = reinterpret_cast<decltype(g_gl_api.BindFramebuffer)>(
      glfwGetProcAddress("glBindFramebuffer"));
  g_gl_api.FramebufferTexture2D = reinterpret_cast<decltype(g_gl_api.FramebufferTexture2D)>(
      glfwGetProcAddress("glFramebufferTexture2D"));
  g_gl_api.GenTextures = reinterpret_cast<decltype(g_gl_api.GenTextures)>(
      glfwGetProcAddress("glGenTextures"));
  g_gl_api.DeleteTextures = reinterpret_cast<decltype(g_gl_api.DeleteTextures)>(
      glfwGetProcAddress("glDeleteTextures"));
  g_gl_api.BindTexture = reinterpret_cast<decltype(g_gl_api.BindTexture)>(
      glfwGetProcAddress("glBindTexture"));
  g_gl_api.TexImage2D = reinterpret_cast<decltype(g_gl_api.TexImage2D)>(
      glfwGetProcAddress("glTexImage2D"));
  g_gl_api.TexParameteri = reinterpret_cast<decltype(g_gl_api.TexParameteri)>(
      glfwGetProcAddress("glTexParameteri"));
#else
  // Non-desktop platforms resolve GL entry points through their native window
  // system (EGL on Android, etc.). Leave every pointer null until a platform
  // native implementation lands under native/gl/ or a sibling backend dir.
#endif
}

void* GetProcAddress(const char* name) {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  return reinterpret_cast<void*>(glfwGetProcAddress(name));
#else
  (void)name;
  return nullptr;
#endif
}

// =============================================================================
// GlTexture
// =============================================================================

GlTexture::~GlTexture() { Release(); }

GlTexture::GlTexture(GlTexture&& other) noexcept
    : handle_(other.handle_),
      allocated_w_(other.allocated_w_),
      allocated_h_(other.allocated_h_) {
  other.handle_ = 0;
  other.allocated_w_ = 0;
  other.allocated_h_ = 0;
}

GlTexture& GlTexture::operator=(GlTexture&& other) noexcept {
  if (this != &other) {
    Release();
    handle_ = other.handle_;
    allocated_w_ = other.allocated_w_;
    allocated_h_ = other.allocated_h_;
    other.handle_ = 0;
    other.allocated_w_ = 0;
    other.allocated_h_ = 0;
  }
  return *this;
}

void GlTexture::Create() {
  if (handle_ != 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.GenTextures == nullptr) {
    return;
  }
  api.GenTextures(1, &handle_);
  if (handle_ == 0) {
    return;
  }
  api.BindTexture(kTexture2d, handle_);
  api.TexParameteri(kTexture2d, kTextureMinFilter, kLinear);
  api.TexParameteri(kTexture2d, kTextureMagFilter, kLinear);
  api.TexParameteri(kTexture2d, kTextureWrapS, kClampToEdge);
  api.TexParameteri(kTexture2d, kTextureWrapT, kClampToEdge);
}

void GlTexture::Bind() const {
  if (handle_ == 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.BindTexture == nullptr) {
    return;
  }
  api.BindTexture(kTexture2d, handle_);
}

void GlTexture::EnsureStorage(int w, int h) {
  if (handle_ == 0) {
    return;
  }
  if (w == allocated_w_ && h == allocated_h_) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.BindTexture == nullptr || api.TexImage2D == nullptr) {
    return;
  }
  api.BindTexture(kTexture2d, handle_);
  api.TexImage2D(kTexture2d, 0, kRgba, w, h, 0, kRgba, kUnsignedByte, nullptr);
  allocated_w_ = w;
  allocated_h_ = h;
}

void GlTexture::Release() {
  if (handle_ == 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.DeleteTextures != nullptr) {
    api.DeleteTextures(1, &handle_);
  }
  handle_ = 0;
  allocated_w_ = 0;
  allocated_h_ = 0;
}

// =============================================================================
// GlFramebuffer
// =============================================================================

GlFramebuffer::~GlFramebuffer() { Release(); }

GlFramebuffer::GlFramebuffer(GlFramebuffer&& other) noexcept : handle_(other.handle_) {
  other.handle_ = 0;
}

GlFramebuffer& GlFramebuffer::operator=(GlFramebuffer&& other) noexcept {
  if (this != &other) {
    Release();
    handle_ = other.handle_;
    other.handle_ = 0;
  }
  return *this;
}

void GlFramebuffer::Create() {
  if (handle_ != 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.GenFramebuffers == nullptr) {
    return;
  }
  api.GenFramebuffers(1, &handle_);
}

void GlFramebuffer::Bind() const {
  if (handle_ == 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.BindFramebuffer == nullptr) {
    return;
  }
  api.BindFramebuffer(kFramebuffer, handle_);
}

void GlFramebuffer::Unbind() {
  GlApi& api = GetGlApi();
  if (api.BindFramebuffer == nullptr) {
    return;
  }
  api.BindFramebuffer(kFramebuffer, 0);
}

void GlFramebuffer::AttachColorTexture(GlUint tex_id) const {
  if (handle_ == 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.BindFramebuffer == nullptr || api.FramebufferTexture2D == nullptr) {
    return;
  }
  api.BindFramebuffer(kFramebuffer, handle_);
  api.FramebufferTexture2D(kFramebuffer, kColorAttachment0, kTexture2d, tex_id, 0);
}

void GlFramebuffer::Release() {
  if (handle_ == 0) {
    return;
  }
  GlApi& api = GetGlApi();
  if (api.DeleteFramebuffers != nullptr) {
    api.DeleteFramebuffers(1, &handle_);
  }
  handle_ = 0;
}

}  // namespace gl
}  // namespace neoflux
