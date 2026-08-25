// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// Renderer backend. When tgfx is available (NEOFLUX_USE_TGFX), this file
// delegates to tgfx for all rendering. Otherwise, a lightweight OpenGL
// fallback renderer is used for desktop platforms.
//
// The fallback path provides:
//   - Colored rectangle rendering via vertex buffer
//   - UTF-8 text rendering via FreeType glyph texture atlas
//   - Transform stack (save/restore/translate) for widget positioning
// =============================================================================

#include "neoflux/render/tgfx_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/font_manager.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
#ifdef NEOFLUX_PLATFORM_DESKTOP
#include <GLFW/glfw3.h>
#endif
#include <ft2build.h>
#include FT_FREETYPE_H

namespace {

// Standard C types for GL function pointers (avoids GL.h dependency).
using GlEnum = unsigned int;
using GlBitfield = unsigned int;
using GlInt = int;
using GlUint = unsigned int;
using GlSizei = int;
using GlFloat = float;
using GlVoid = void;
using GlSizeiptr = long long;  // NOLINT
using GlIntptr = long long;    // NOLINT

// ---------------------------------------------------------------------------
// OpenGL function loader. Loads all required entry points via
// glfwGetProcAddress after a context is current.
// ---------------------------------------------------------------------------
struct GlLoader {
  void(APIENTRY* glEnable)(GlEnum) = nullptr;
  void(APIENTRY* glDisable)(GlEnum) = nullptr;
  void(APIENTRY* glBlendFunc)(GlEnum, GlEnum) = nullptr;
  void(APIENTRY* glViewport)(GlInt, GlInt, GlSizei, GlSizei) = nullptr;
  void(APIENTRY* glClearColor)(GlFloat, GlFloat, GlFloat, GlFloat) = nullptr;
  void(APIENTRY* glClear)(GlBitfield) = nullptr;
  GlUint(APIENTRY* glCreateShader)(GlEnum) = nullptr;
  void(APIENTRY* glShaderSource)(GlUint, GlSizei, const char* const*,
                                 const GlInt*) = nullptr;
  void(APIENTRY* glCompileShader)(GlUint) = nullptr;
  void(APIENTRY* glGetShaderiv)(GlUint, GlEnum, GlInt*) = nullptr;
  void(APIENTRY* glGetShaderInfoLog)(GlUint, GlSizei, GlSizei*, char*) = nullptr;
  GlUint(APIENTRY* glCreateProgram)() = nullptr;
  void(APIENTRY* glAttachShader)(GlUint, GlUint) = nullptr;
  void(APIENTRY* glLinkProgram)(GlUint) = nullptr;
  void(APIENTRY* glGetProgramiv)(GlUint, GlEnum, GlInt*) = nullptr;
  void(APIENTRY* glGetProgramInfoLog)(GlUint, GlSizei, GlSizei*, char*) = nullptr;
  void(APIENTRY* glDeleteShader)(GlUint) = nullptr;
  GlInt(APIENTRY* glGetUniformLocation)(GlUint, const char*) = nullptr;
  void(APIENTRY* glUseProgram)(GlUint) = nullptr;
  void(APIENTRY* glUniform4f)(GlInt, GlFloat, GlFloat, GlFloat, GlFloat) = nullptr;
  void(APIENTRY* glUniform2f)(GlInt, GlFloat, GlFloat) = nullptr;
  void(APIENTRY* glUniform1i)(GlInt, GlInt) = nullptr;
  void(APIENTRY* glGenVertexArrays)(GlSizei, GlUint*) = nullptr;
  void(APIENTRY* glGenBuffers)(GlSizei, GlUint*) = nullptr;
  void(APIENTRY* glBindVertexArray)(GlUint) = nullptr;
  void(APIENTRY* glBindBuffer)(GlEnum, GlUint) = nullptr;
  void(APIENTRY* glEnableVertexAttribArray)(GlUint) = nullptr;
  void(APIENTRY* glVertexAttribPointer)(GlUint, GlInt, GlEnum, unsigned char,
                                        GlSizei, const GlVoid*) = nullptr;
  void(APIENTRY* glBufferData)(GlEnum, GlSizeiptr, const GlVoid*, GlEnum) = nullptr;
  void(APIENTRY* glBufferSubData)(GlEnum, GlIntptr, GlSizeiptr, const GlVoid*) = nullptr;
  void(APIENTRY* glDrawArrays)(GlEnum, GlInt, GlSizei) = nullptr;
  void(APIENTRY* glGenTextures)(GlSizei, GlUint*) = nullptr;
  void(APIENTRY* glBindTexture)(GlEnum, GlUint) = nullptr;
  void(APIENTRY* glTexParameteri)(GlEnum, GlEnum, GlInt) = nullptr;
  void(APIENTRY* glTexImage2D)(GlEnum, GlInt, GlInt, GlSizei, GlSizei, GlInt,
                               GlEnum, GlEnum, const GlVoid*) = nullptr;
  void(APIENTRY* glTexSubImage2D)(GlEnum, GlInt, GlInt, GlInt, GlSizei, GlSizei,
                                  GlEnum, GlEnum, const GlVoid*) = nullptr;
  void(APIENTRY* glActiveTexture)(GlEnum) = nullptr;
  void(APIENTRY* glPixelStorei)(GlEnum, GlInt) = nullptr;
  void(APIENTRY* glScissor)(GlInt, GlInt, GlSizei, GlSizei) = nullptr;
  void(APIENTRY* glDeleteTextures)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteBuffers)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteVertexArrays)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteProgram)(GlUint) = nullptr;
  GlEnum(APIENTRY* glGetError)() = nullptr;

  bool Load() {
// NOLINTBEGIN(bugprone-macro-parentheses)
#ifdef NEOFLUX_PLATFORM_DESKTOP
#define NEOFLUX_GL_GET_PROC(name) glfwGetProcAddress(name)
#elif defined(ANDROID)
#include <EGL/egl.h>
#define NEOFLUX_GL_GET_PROC(name) eglGetProcAddress(name)
#else
// iOS: EAGL contexts provide GL functions directly; dlsym as fallback.
#include <dlfcn.h>
#define NEOFLUX_GL_GET_PROC(name) dlsym(RTLD_DEFAULT, name)
#endif
#define NEOFLUX_LOAD_GL(name)                                       \
  name = reinterpret_cast<decltype(name)>(NEOFLUX_GL_GET_PROC(#name)); \
  if (name == nullptr) {                                            \
    LOG(ERROR) << "Failed to load " << #name;                       \
    return false;                                                   \
  }
  // NOLINTEND(bugprone-macro-parentheses)
  // NOLINTBEGIN(bugprone-macro-parentheses)
  NEOFLUX_LOAD_GL(glEnable)
  NEOFLUX_LOAD_GL(glDisable)
  NEOFLUX_LOAD_GL(glBlendFunc)
    NEOFLUX_LOAD_GL(glViewport)
    NEOFLUX_LOAD_GL(glClearColor)
    NEOFLUX_LOAD_GL(glClear)
    NEOFLUX_LOAD_GL(glCreateShader)
    NEOFLUX_LOAD_GL(glShaderSource)
    NEOFLUX_LOAD_GL(glCompileShader)
    NEOFLUX_LOAD_GL(glGetShaderiv)
    NEOFLUX_LOAD_GL(glGetShaderInfoLog)
    NEOFLUX_LOAD_GL(glCreateProgram)
    NEOFLUX_LOAD_GL(glAttachShader)
    NEOFLUX_LOAD_GL(glLinkProgram)
    NEOFLUX_LOAD_GL(glGetProgramiv)
    NEOFLUX_LOAD_GL(glGetProgramInfoLog)
    NEOFLUX_LOAD_GL(glDeleteShader)
    NEOFLUX_LOAD_GL(glGetUniformLocation)
    NEOFLUX_LOAD_GL(glUseProgram)
    NEOFLUX_LOAD_GL(glUniform4f)
    NEOFLUX_LOAD_GL(glUniform2f)
    NEOFLUX_LOAD_GL(glUniform1i)
    NEOFLUX_LOAD_GL(glGenVertexArrays)
    NEOFLUX_LOAD_GL(glGenBuffers)
    NEOFLUX_LOAD_GL(glBindVertexArray)
    NEOFLUX_LOAD_GL(glBindBuffer)
    NEOFLUX_LOAD_GL(glEnableVertexAttribArray)
    NEOFLUX_LOAD_GL(glVertexAttribPointer)
    NEOFLUX_LOAD_GL(glBufferData)
    NEOFLUX_LOAD_GL(glBufferSubData)
    NEOFLUX_LOAD_GL(glDrawArrays)
    NEOFLUX_LOAD_GL(glGenTextures)
    NEOFLUX_LOAD_GL(glBindTexture)
    NEOFLUX_LOAD_GL(glTexParameteri)
    NEOFLUX_LOAD_GL(glTexImage2D)
    NEOFLUX_LOAD_GL(glTexSubImage2D)
    NEOFLUX_LOAD_GL(glActiveTexture)
    NEOFLUX_LOAD_GL(glPixelStorei)
    NEOFLUX_LOAD_GL(glScissor)
    NEOFLUX_LOAD_GL(glDeleteTextures)
    NEOFLUX_LOAD_GL(glDeleteBuffers)
    NEOFLUX_LOAD_GL(glDeleteVertexArrays)
    NEOFLUX_LOAD_GL(glDeleteProgram)
    NEOFLUX_LOAD_GL(glGetError)
  // NOLINTEND(bugprone-macro-parentheses)
#undef NEOFLUX_LOAD_GL
#undef NEOFLUX_GL_GET_PROC
    return true;
  }
};

// Vertex format: position (x, y) + texcoord (u, v), 4 floats per vertex.
constexpr int kVertexSize = 4;

// Pre-allocated VBO capacity in bytes. Large enough for a full rounded-rect
// fan (~42 vertices) or a long text string (~256 glyphs).
constexpr GlSizeiptr kMaxVboBytes = 64LL * 1024;

// Texture atlas dimensions for glyph caching.
constexpr int kAtlasSize = 1024;

// Color normalization scale: 8-bit channel (0-255) -> float (0.0-1.0).
// Precompute reciprocal so each conversion is a multiply (~4 cycles) instead
// of a divide (~14 cycles). Used for every glUniform4f color upload.
constexpr float kColorScale = 1.0F / 255.0F;

// Per-glyph metadata stored in the atlas cache.
struct GlyphInfo {
  float u0 = 0.0F;
  float v0 = 0.0F;
  float u1 = 0.0F;
  float v1 = 0.0F;
  float bearing_x = 0.0F;
  float bearing_y = 0.0F;
  float advance = 0.0F;
  int width = 0;
  int height = 0;
};

// Transform entry: accumulated (x, y) translation.
struct Transform {
  float x = 0.0F;
  float y = 0.0F;
};

// ---------------------------------------------------------------------------
// Lightweight OpenGL fallback renderer.
// ---------------------------------------------------------------------------
class GlRendererImpl : public NonCopyable {
 public:
  GlRendererImpl() = default;
  ~GlRendererImpl() { Cleanup(); }
  GlRendererImpl(const GlRendererImpl&) = delete;
  GlRendererImpl& operator=(const GlRendererImpl&) = delete;
  GlRendererImpl(GlRendererImpl&&) = delete;
  GlRendererImpl& operator=(GlRendererImpl&&) = delete;

  bool Init(int width, int height, std::string_view font_dir,
            void* native_handle) {
    window_ = static_cast<GLFWwindow*>(native_handle);
    width_ = width;
    height_ = height;
    // GL initialization is deferred to BeginFrame, which runs on the render
    // thread where the GL context is current. FreeType does not need GL.
    InitFonts(font_dir);
    return true;
  }

  void BeginFrame(const Color& clear) {
    if (!InitializeGL()) {
      return;
    }
    // Query the actual framebuffer size (may differ from window size due to
    // DPI scaling). glViewport scales NDC to framebuffer pixels; u_resolution
    // stays at layout (window) size so shader math uses layout coordinates.
    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(window_, &fb_width, &fb_height);
    if (fb_width <= 0 || fb_height <= 0) {
      fb_width = width_;
      fb_height = height_;
    }
    gl.glViewport(0, 0, fb_width, fb_height);
    // Query the logical window size every frame so resizes are reflected in
    // the shader's u_resolution without a separate callback.
    int win_width = 0;
    int win_height = 0;
    glfwGetWindowSize(window_, &win_width, &win_height);
    if (win_width > 0 && win_height > 0) {
      width_ = win_width;
      height_ = win_height;
    }
    gl.glClearColor(clear.r * kColorScale, clear.g * kColorScale, clear.b * kColorScale,
                    clear.a * kColorScale);
    gl.glClear(0x00004000U | 0x00000100U);
    // Update u_resolution every frame so window resizes take effect
    // without re-initializing GL. u_resolution uses logical (layout) size,
    // not physical framebuffer size.
    gl.glUseProgram(program_);
    current_program_ = program_;
    current_use_texture_ = -1;  // force uniform update on first draw
    gl.glUniform2f(u_resolution_, static_cast<float>(width_),
                   static_cast<float>(height_));
    transform_stack_.clear();
    transform_stack_.emplace_back(0.0F, 0.0F);
    clip_stack_.clear();
  }

  void EndFrame() { /* buffer swap handled by GLFW bridge */ }

  void DrawRect(const Rect& rect, const Color& color) {
    if (!gl_ready_) {
      return;
    }
    const Transform& t = transform_stack_.back();

    // Use relative vertices and u_translate for positioning.
    const float vertices[] = {
      rect.x, rect.y, 0.0F, 0.0F,
      rect.x + rect.width, rect.y, 0.0F, 0.0F,
      rect.x, rect.y + rect.height, 0.0F, 0.0F,
      rect.x + rect.width, rect.y, 0.0F, 0.0F,
      rect.x + rect.width, rect.y + rect.height, 0.0F, 0.0F,
      rect.x, rect.y + rect.height, 0.0F, 0.0F,
    };

    UseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r * kColorScale, color.g * kColorScale,
                   color.b * kColorScale, color.a * kColorScale);
    if (current_use_texture_ != 0) {
      gl.glUniform1i(u_use_texture_, 0);
      current_use_texture_ = 0;
    }
    BindVertexArray(vao_);
    BindBuffer(vbo_);
    gl.glBufferSubData(0x8892, 0, sizeof(vertices), vertices);
    gl.glDrawArrays(0x0004, 0, 6);
  }

  // Draws a filled rounded rectangle using a triangle fan. The boundary
  // is sampled at kRoundedSegments points per corner; interior is filled
  // from the rectangle centre. Coordinates use y-down (screen space), so
  // the y-component of each arc uses -sin(angle).
  void DrawRoundedRect(const Rect& rect, const Color& color, float radius) {
    if (!gl_ready_) {
      return;
    }
    if (radius <= 0.0F || rect.width <= 0.0F || rect.height <= 0.0F) {
      DrawRect(rect, color);
      return;
    }
    const float r = std::min(radius, std::min(rect.width, rect.height) * 0.5F);
    const Transform& t = transform_stack_.back();
    constexpr int kSeg = 10;
    // 1 centre + 4 corners * kSeg boundary points (last point of each
    // corner is shared with first point of next, so we skip the duplicate
    // on corners 1-3 and add a final closing point).
    constexpr int kBoundary = (4 * kSeg) + 1;
    constexpr int kVertexCount = 1 + kBoundary;
    float vertices[kVertexCount * 4];
    const float cx = rect.x + (rect.width * 0.5F);
    const float cy = rect.y + (rect.height * 0.5F);
    int idx = 0;
    vertices[idx++] = cx;
    vertices[idx++] = cy;
    vertices[idx++] = 0.0F;
    vertices[idx++] = 0.0F;
    // Corner centres in order TL, TR, BR, BL. Each corner sweeps a
    // quarter-circle clockwise (visually CCW in y-down space).
    constexpr float kPi = std::numbers::pi_v<float>;
    struct Corner {
      float cxy[2];
      float a0;
    };
    const Corner corners[4] = {
        {.cxy = {rect.x + r, rect.y + r}, .a0 = kPi},
        {.cxy = {rect.x + rect.width - r, rect.y + r}, .a0 = kPi * 0.5F},
        {.cxy = {rect.x + rect.width - r, rect.y + rect.height - r}, .a0 = 0.0F},
        {.cxy = {rect.x + r, rect.y + rect.height - r}, .a0 = -kPi * 0.5F},
    };
    for (const Corner& corner : corners) {
      for (int s = 0; s < kSeg; ++s) {
        const float a = corner.a0 -
                        ((kPi * 0.5F) * static_cast<float>(s) /
                         static_cast<float>(kSeg));
        const float px = corner.cxy[0] + (r * std::cos(a));
        const float py = corner.cxy[1] - (r * std::sin(a));
        vertices[idx++] = px;
        vertices[idx++] = py;
        vertices[idx++] = 0.0F;
        vertices[idx++] = 0.0F;
      }
    }
    // Closing point = first boundary point (TL left end).
    vertices[idx++] = rect.x;
    vertices[idx++] = rect.y + r;
    vertices[idx++] = 0.0F;
    vertices[idx++] = 0.0F;

    UseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r * kColorScale, color.g * kColorScale,
                   color.b * kColorScale, color.a * kColorScale);
    if (current_use_texture_ != 0) {
      gl.glUniform1i(u_use_texture_, 0);
      current_use_texture_ = 0;
    }
    BindVertexArray(vao_);
    BindBuffer(vbo_);
    gl.glBufferSubData(0x8892, 0, sizeof(vertices), vertices);
    gl.glDrawArrays(0x0006, 0, kVertexCount);  // GL_TRIANGLE_FAN
  }

  void DrawText(std::string_view text, const Point& position,
                const Color& color, float font_size,
                std::string_view font_name) {
    if (!gl_ready_) {
      return;
    }
    const Transform& t = transform_stack_.back();
    FT_Face face = GetFontFace(font_name);
    if (face == nullptr) {
      return;
    }
    float cursor_x = position.x;
    const float baseline_y = position.y;

    UseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r * kColorScale, color.g * kColorScale,
                   color.b * kColorScale, color.a * kColorScale);
    if (current_use_texture_ != 1) {
      gl.glUniform1i(u_use_texture_, 1);
      current_use_texture_ = 1;
    }
    gl.glActiveTexture(0x84C0);  // GL_TEXTURE0
    BindTexture(atlas_texture_);
    BindVertexArray(vao_);
    BindBuffer(vbo_);

    std::size_t i = 0;
    while (i < text.size()) {
      std::uint32_t cp = 0;
      const auto c = static_cast<unsigned char>(text[i]);
      int seq_len = 1;
      if (c < 0x80) {
        cp = c;
      } else if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F;
        seq_len = 2;
      } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F;
        seq_len = 3;
      } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07;
        seq_len = 4;
      }
      for (int j = 1; j < seq_len && i + j < text.size(); ++j) {
        cp = (cp << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3FU);
      }
      i += seq_len;

      const GlyphInfo* glyph = GetGlyph(face, cp, font_size);
      if (glyph == nullptr) {
        continue;
      }

      const float draw_x = cursor_x + glyph->bearing_x;
      const float draw_y = baseline_y - glyph->bearing_y;
      const float x0 = draw_x;
      const float y0 = draw_y;
      const float x1 = draw_x + static_cast<float>(glyph->width);
      const float y1 = draw_y + static_cast<float>(glyph->height);

      const float vertices[] = {
        x0, y0, glyph->u0, glyph->v0, x1, y0, glyph->u1, glyph->v0,
        x0, y1, glyph->u0, glyph->v1, x1, y0, glyph->u1, glyph->v0,
        x1, y1, glyph->u1, glyph->v1, x0, y1, glyph->u0, glyph->v1,
      };

      gl.glBufferSubData(0x8892, 0, sizeof(vertices), vertices);
      gl.glDrawArrays(0x0004, 0, 6);

      cursor_x += glyph->advance;
    }
  }

  // Draws an external GL texture into the given rectangle. Used for video
  // playback (libmpv output) and other externally-generated textures.
  // Texture is flipped vertically because most video APIs output frames with
  // origin at bottom-left, while NeoFlux uses top-left origin.
  void DrawTexture(std::uint32_t texture_id, const Rect& rect) {
    if (!gl_ready_ || texture_id == 0) {
      return;
    }
    const Transform& t = transform_stack_.back();
    UseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    // White color: texture provides RGB, multiply by 1.0.
    gl.glUniform4f(u_color_, 1.0F, 1.0F, 1.0F, 1.0F);
    if (current_use_texture_ != 1) {
      gl.glUniform1i(u_use_texture_, 1);
      current_use_texture_ = 1;
    }
    gl.glActiveTexture(0x84C0);  // GL_TEXTURE0
    BindTexture(texture_id);
    BindVertexArray(vao_);
    BindBuffer(vbo_);

    // Texture coordinates: flip Y (0,1 top-left -> 1,0 bottom-left in GL).
    const float vertices[] = {
      rect.x, rect.y,                             0.0F, 1.0F,
      rect.x + rect.width, rect.y,                1.0F, 1.0F,
      rect.x, rect.y + rect.height,               0.0F, 0.0F,
      rect.x + rect.width, rect.y,                1.0F, 1.0F,
      rect.x + rect.width, rect.y + rect.height,  1.0F, 0.0F,
      rect.x, rect.y + rect.height,               0.0F, 0.0F,
    };
    gl.glBufferSubData(0x8892, 0, sizeof(vertices), vertices);
    gl.glDrawArrays(0x0004, 0, 6);  // GL_TRIANGLES
  }

  void Save() {
    transform_stack_.push_back(transform_stack_.empty()
                                   ? Transform{0.0F, 0.0F}
                                   : transform_stack_.back());
    // Save current clip rect (width < 0 means no clipping).
    clip_stack_.push_back(clip_stack_.empty()
                              ? Rect{.x = 0, .y = 0, .width = -1.0F, .height = -1.0F}
                              : clip_stack_.back());
  }

  void Restore() {
    if (transform_stack_.size() > 1) {
      transform_stack_.pop_back();
    }
    if (clip_stack_.size() > 1) {
      clip_stack_.pop_back();
      ApplyClip(clip_stack_.back());
    }
  }

  void Translate(float delta_x, float delta_y) {
    if (!transform_stack_.empty()) {
      transform_stack_.back().x += delta_x;
      transform_stack_.back().y += delta_y;
    }
  }

  void ClipRect(const Rect& rect) {
    // Intersect with current clip (if any) and apply.
    Rect result = rect;
    if (!clip_stack_.empty() && clip_stack_.back().width >= 0.0F) {
      const Rect& cur = clip_stack_.back();
      const float x1 = std::max(cur.x, rect.x);
      const float y1 = std::max(cur.y, rect.y);
      const float x2 = std::min(cur.x + cur.width, rect.x + rect.width);
      const float y2 = std::min(cur.y + cur.height, rect.y + rect.height);
      result = {.x = x1, .y = y1, .width = std::max(0.0F, x2 - x1),
                .height = std::max(0.0F, y2 - y1)};
    }
    // NOLINTNEXTLINE(bugprone-branch-clone)
    if (clip_stack_.empty()) {
      clip_stack_.push_back(result);
    } else {
      clip_stack_.back() = result;
    }
    ApplyClip(result);
  }

  // Applies a clip rect via glScissor. rect.width < 0 disables scissor.
  void ApplyClip(const Rect& rect) const {
    if (rect.width < 0.0F || rect.height < 0.0F) {
      gl.glDisable(0x0C11);  // GL_SCISSOR_TEST
      return;
    }
    gl.glEnable(0x0C11);  // GL_SCISSOR_TEST
    // glScissor uses bottom-left origin; our coords are top-left.
    gl.glScissor(static_cast<GlInt>(rect.x),
                 static_cast<GlInt>(static_cast<float>(height_) - rect.y -
                                    rect.height),
                 static_cast<GlSizei>(rect.width),
                 static_cast<GlSizei>(rect.height));
  }

  void Resize(int width, int height) {
    width_ = width;
    height_ = height;
  }

 private:
  bool InitializeGL() {
    if (gl_ready_) {
      return true;
    }
    if (!gl.Load()) {
      LOG(ERROR) << "Failed to load OpenGL functions";
      return false;
    }

    // Shader version differs between desktop OpenGL (330 core) and OpenGL ES
    // (300 es + precision qualifier). The rest of the shader is compatible.
#ifdef NEOFLUX_PLATFORM_MOBILE
    static const char* kGlVersion = "#version 300 es\nprecision mediump float;\n";
#else
    static const char* kGlVersion = "#version 330 core\n";
#endif

    static const char* kVertBody = R"(
      layout(location = 0) in vec4 a_pos;
      uniform vec2 u_resolution;
      uniform vec2 u_translate;
      out vec2 v_uv;
      void main() {
        vec2 p = a_pos.xy + u_translate;
        vec2 clip = (p / u_resolution) * 2.0 - 1.0;
        gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
        v_uv = a_pos.zw;
      })";
    static const char* kFragBody = R"(
      in vec2 v_uv;
      uniform vec4 u_color;
      uniform sampler2D u_texture;
      uniform int u_use_texture;
      out vec4 frag_color;
      void main() {
        if (u_use_texture != 0) {
          float a = texture(u_texture, v_uv).r;
          frag_color = vec4(u_color.rgb, u_color.a * a);
        } else {
          frag_color = u_color;
        }
      })";

    // Concatenate version header + body.
    std::string vert_src = std::string(kGlVersion) + kVertBody;
    std::string frag_src = std::string(kGlVersion) + kFragBody;
    const char* vert_ptr = vert_src.c_str();
    const char* frag_ptr = frag_src.c_str();

    const GlUint vs = gl.glCreateShader(0x8B31);
    const GlUint fs = gl.glCreateShader(0x8B30);
    gl.glShaderSource(vs, 1, &vert_ptr, nullptr);
    gl.glShaderSource(fs, 1, &frag_ptr, nullptr);
    gl.glCompileShader(vs);
    gl.glCompileShader(fs);

    GlInt ok = 0;
    gl.glGetShaderiv(vs, 0x8B81, &ok);
    if (ok == 0) {
      char log[512];
      gl.glGetShaderInfoLog(vs, 512, nullptr, log);
      LOG(ERROR) << "Vertex shader compile failed: " << log;
      return false;
    }
    gl.glGetShaderiv(fs, 0x8B81, &ok);
    if (ok == 0) {
      char log[512];
      gl.glGetShaderInfoLog(fs, 512, nullptr, log);
      LOG(ERROR) << "Fragment shader compile failed: " << log;
      return false;
    }

    program_ = gl.glCreateProgram();
    gl.glAttachShader(program_, vs);
    gl.glAttachShader(program_, fs);
    gl.glLinkProgram(program_);
    gl.glGetProgramiv(program_, 0x8B82, &ok);
    if (ok == 0) {
      char log[512];
      gl.glGetProgramInfoLog(program_, 512, nullptr, log);
      LOG(ERROR) << "Shader link failed: " << log;
      return false;
    }
    gl.glDeleteShader(vs);
    gl.glDeleteShader(fs);

    u_resolution_ = gl.glGetUniformLocation(program_, "u_resolution");
    u_translate_ = gl.glGetUniformLocation(program_, "u_translate");
    u_color_ = gl.glGetUniformLocation(program_, "u_color");
    u_texture_ = gl.glGetUniformLocation(program_, "u_texture");
    u_use_texture_ = gl.glGetUniformLocation(program_, "u_use_texture");

    gl.glGenVertexArrays(1, &vao_);
    gl.glGenBuffers(1, &vbo_);
    gl.glBindVertexArray(vao_);
    gl.glBindBuffer(0x8892, vbo_);
    // Pre-allocate VBO storage. Using glBufferSubData per-draw avoids
    // repeated reallocation (which can stall on first use) and is the
    // recommended path for dynamic vertex data.
    gl.glBufferData(0x8892, kMaxVboBytes, nullptr, 0x88E8);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, kVertexSize, 0x1406, 0,
                             kVertexSize * static_cast<GlSizei>(sizeof(float)),
                             nullptr);

    gl.glGenTextures(1, &atlas_texture_);
    gl.glBindTexture(0x0DE1, atlas_texture_);
    gl.glTexParameteri(0x0DE1, 0x2800, 0x2601);
    gl.glTexParameteri(0x0DE1, 0x2801, 0x2601);
    gl.glTexParameteri(0x0DE1, 0x2802, 0x812F);
    gl.glTexParameteri(0x0DE1, 0x2803, 0x812F);
    // GL_R8 internal format: 8-bit single-channel grayscale for glyph coverage.
    gl.glTexImage2D(0x0DE1, 0, 0x8229, kAtlasSize, kAtlasSize, 0, 0x1903,
                    0x1401, nullptr);
    // Clear the atlas to zero (transparent). glTexImage2D with nullptr
    // leaves the texture contents undefined, which can cause garbage
    // pixels (white squares) on first frame before any glyph is uploaded.
    {
      std::vector<unsigned char> zeros(
          static_cast<std::size_t>(kAtlasSize) * kAtlasSize, 0);
      gl.glTexSubImage2D(0x0DE1, 0, 0, 0, kAtlasSize, kAtlasSize, 0x1903,
                         0x1401, zeros.data());
    }

    gl.glEnable(0x0BE2);
    gl.glBlendFunc(0x0302, 0x0303);

    gl.glUseProgram(program_);
    gl.glUniform2f(u_resolution_, static_cast<float>(width_),
                   static_cast<float>(height_));
    gl.glUniform2f(u_translate_, 0.0F, 0.0F);
    gl.glUniform1i(u_texture_, 0);

    gl_ready_ = true;
    LOG(INFO) << "OpenGL fallback renderer initialized (" << width_ << "x"
              << height_ << ")";
    return true;
  }

  // Initializes FreeType and scans the configured font directory.
  // `font_dir` is the directory to scan for .ttf/.otf/.ttc files. Relative
  // paths are searched from the working directory, then upward (../, ../../)
  // to handle build subdirectories.
  void InitFonts(std::string_view font_dir) {
    if (FT_Init_FreeType(&ft_library_) != 0) {
      LOG(ERROR) << "Failed to initialize FreeType";
      return;
    }
    const std::string dir(font_dir);
    font_manager_.ScanDirectory(dir);
    font_manager_.ScanDirectory("../" + dir);
    font_manager_.ScanDirectory("../../" + dir);
    if (font_manager_.GetFontCount() == 0) {
      LOG(WARNING) << "No fonts found in '" << dir
                   << "' (or parent directories). "
                   << "Text rendering will be disabled. "
                   << "Call Application::SetFontDir() before Init() to specify "
                   << "a font directory.";
    }
  }

  // Returns the FT_Face for the given font name, loading and caching it on
  // first use. If font_name is empty, returns the default font. Returns
  // nullptr if the font cannot be found or loaded.
  FT_Face GetFontFace(std::string_view font_name) {
    if (ft_library_ == nullptr) {
      return nullptr;
    }
    std::string name(font_name);
    if (name.empty()) {
      name = font_manager_.GetDefaultFont();
    }
    if (name.empty()) {
      return nullptr;
    }
    // Normalize to lowercase for case-insensitive cache lookup (matches
    // FontManager's internal keying).
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const auto it = font_faces_.find(name);
    if (it != font_faces_.end()) {
      return it->second;
    }

    const std::string path = font_manager_.GetPath(name);
    if (path.empty()) {
      LOG(WARNING) << "Font not found: " << name;
      return nullptr;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(ft_library_, path.c_str(), 0, &face) != 0) {
      LOG(ERROR) << "Failed to load font: " << path;
      return nullptr;
    }
    font_faces_[name] = face;
    LOG(INFO) << "Loaded font: " << name << " (" << path << ")";
    return face;
  }

  const GlyphInfo* GetGlyph(FT_Face face, std::uint32_t codepoint,
                            float font_size) {
    if (face == nullptr) {
      return nullptr;
    }

    const int size_key = static_cast<int>(font_size);
    const auto key = (static_cast<std::uint64_t>(codepoint) << 32) |
                     static_cast<std::uint32_t>(size_key);
    // Fast path: single-entry cache for the most recently looked-up glyph.
    // Avoids unordered_map hash+lookup for repeated characters in text.
    if (key == last_glyph_key_ && last_glyph_ptr_ != nullptr) {
      return last_glyph_ptr_;
    }

    auto it = glyph_cache_.find(key);
    if (it != glyph_cache_.end()) {
      last_glyph_key_ = key;
      last_glyph_ptr_ = &it->second;
      return last_glyph_ptr_;
    }

    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(size_key));

    if (FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT) != 0) {
      return nullptr;
    }
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
      return nullptr;
    }

    const FT_Bitmap& bitmap = face->glyph->bitmap;
    const int w = static_cast<int>(bitmap.width);
    const int h = static_cast<int>(bitmap.rows);

    if (w == 0 || h == 0) {
      GlyphInfo info;
      info.advance = static_cast<float>(
          static_cast<std::uint32_t>(face->glyph->advance.x) >> 6U);  // NOLINT(bugprone-signed-bitwise)
      glyph_cache_[key] = info;
      last_glyph_key_ = key;
      last_glyph_ptr_ = &glyph_cache_[key];
      return last_glyph_ptr_;
    }

    if (atlas_x_ + w > kAtlasSize) {
      atlas_x_ = 0;
      atlas_y_ = atlas_row_height_ + 1;
      atlas_row_height_ = 0;
    }
    if (atlas_y_ + h > kAtlasSize) {
      LOG(WARNING) << "Glyph atlas full";
      return nullptr;
    }

    // Copy glyph bitmap to a tightly packed grayscale buffer. FreeType may
    // pad rows (pitch > width) or use negative pitch (bottom-up).
    std::vector<unsigned char> packed(static_cast<std::size_t>(w) * h);
    const int pitch = bitmap.pitch;
    if (pitch >= 0) {
      for (int row = 0; row < h; ++row) {
        std::memcpy(packed.data() + static_cast<std::size_t>(row) * w,
                    bitmap.buffer + row * pitch,
                    static_cast<std::size_t>(w));
      }
    } else {
      // Negative pitch: bitmap is bottom-up. Read from bottom row.
      for (int row = 0; row < h; ++row) {
        std::memcpy(packed.data() + static_cast<std::size_t>(row) *
                                        static_cast<std::size_t>(w),
                    bitmap.buffer + (h - 1 - row) * (-pitch),
                    static_cast<std::size_t>(w));
      }
    }

    BindTexture(atlas_texture_);
    gl.glPixelStorei(0x0CF5, 1);  // GL_UNPACK_ALIGNMENT = 1
    gl.glTexSubImage2D(0x0DE1, 0, atlas_x_, atlas_y_, w, h, 0x1903, 0x1401,
                       packed.data());
    gl.glPixelStorei(0x0CF5, 4);  // reset to default

    GlyphInfo info;
    info.u0 = static_cast<float>(atlas_x_) / kAtlasSize;
    info.v0 = static_cast<float>(atlas_y_) / kAtlasSize;
    info.u1 = static_cast<float>(atlas_x_ + w) / kAtlasSize;
    info.v1 = static_cast<float>(atlas_y_ + h) / kAtlasSize;
    info.bearing_x = static_cast<float>(face->glyph->bitmap_left);
    info.bearing_y = static_cast<float>(face->glyph->bitmap_top);
    info.advance = static_cast<float>(
        static_cast<std::uint32_t>(face->glyph->advance.x) >> 6U);  // NOLINT(bugprone-signed-bitwise)
    info.width = w;
    info.height = h;
    glyph_cache_[key] = info;

    atlas_x_ += w + 1;
    atlas_row_height_ = std::max(atlas_row_height_, h);

    last_glyph_key_ = key;
    last_glyph_ptr_ = &glyph_cache_[key];
    return last_glyph_ptr_;
  }

  void Cleanup() {
    if (gl_ready_) {
      gl.glDeleteTextures(1, &atlas_texture_);
      gl.glDeleteBuffers(1, &vbo_);
      gl.glDeleteVertexArrays(1, &vao_);
      gl.glDeleteProgram(program_);
    }
    for (auto& [name, face] : font_faces_) {
      if (face != nullptr) {
        FT_Done_Face(face);
      }
    }
    font_faces_.clear();
    if (ft_library_ != nullptr) {
      FT_Done_FreeType(ft_library_);
    }
  }

  GlLoader gl;
  GLFWwindow* window_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  bool gl_ready_ = false;

  // Cached GL state to avoid redundant state-change calls (each gl* call
  // may trigger an ioctl into the GPU driver, ~33% of CPU in profiling).
  GlUint current_program_ = 0;
  GlUint current_texture_ = 0;
  GlUint current_vao_ = 0;
  GlUint current_vbo_ = 0;
  GlInt current_use_texture_ = -1;

  // Binds a program only if different from the currently bound one.
  void UseProgram(GlUint program) {
    if (program != current_program_) {
      gl.glUseProgram(program);
      current_program_ = program;
    }
  }

  // Binds a texture only if different from the currently bound one.
  void BindTexture(GlUint texture) {
    if (texture != current_texture_) {
      gl.glBindTexture(0x0DE1, texture);  // GL_TEXTURE_2D
      current_texture_ = texture;
    }
  }

  // Binds a VAO only if different from the currently bound one.
  void BindVertexArray(GlUint vao) {
    if (vao != current_vao_) {
      gl.glBindVertexArray(vao);
      current_vao_ = vao;
    }
  }

  // Binds a VBO only if different from the currently bound one.
  void BindBuffer(GlUint vbo) {
    if (vbo != current_vbo_) {
      gl.glBindBuffer(0x8892, vbo);  // GL_ARRAY_BUFFER
      current_vbo_ = vbo;
    }
  }

  GlUint program_ = 0;
  GlUint vao_ = 0;
  GlUint vbo_ = 0;
  GlUint atlas_texture_ = 0;

  GlInt u_resolution_ = -1;
  GlInt u_translate_ = -1;
  GlInt u_color_ = -1;
  GlInt u_texture_ = -1;
  GlInt u_use_texture_ = -1;

  FT_Library ft_library_ = nullptr;
  FontManager font_manager_{};
  std::unordered_map<std::string, FT_Face> font_faces_{};

  std::unordered_map<std::uint64_t, GlyphInfo> glyph_cache_{};
  // Single-entry fast cache for the most recently looked-up glyph. Text
  // rendering often repeats characters (e.g. spaces, common letters) at the
  // same font size; this avoids an unordered_map lookup (~0.4% of CPU in
  // profiling) for repeated codepoints.
  std::uint64_t last_glyph_key_ = 0;
  const GlyphInfo* last_glyph_ptr_ = nullptr;
  int atlas_x_ = 0;
  int atlas_y_ = 0;
  int atlas_row_height_ = 0;

  std::vector<Transform> transform_stack_{};
  std::vector<Rect> clip_stack_{};
};

}  // namespace

#endif  // NEOFLUX_PLATFORM_DESKTOP || NEOFLUX_PLATFORM_MOBILE

// =============================================================================
// TgfxRenderer public interface
// =============================================================================

TgfxRenderer::TgfxRenderer() = default;

TgfxRenderer::~TgfxRenderer() {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  delete static_cast<GlRendererImpl*>(impl_);
#endif
}

bool TgfxRenderer::Init(int width, int height, std::string_view font_dir,
                        void* native_handle) {
  if (initialized_) {
    return true;
  }
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  auto* impl = new GlRendererImpl();
  if (!impl->Init(width, height, font_dir, native_handle)) {
    delete impl;
    return false;
  }
  impl_ = impl;
  width_ = width;
  height_ = height;
  initialized_ = true;
  return true;
#else
  (void)width;
  (void)height;
  (void)native_handle;
  return false;
#endif
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->BeginFrame(clear_color);
  }
#else
  (void)clear_color;
#endif
}

void TgfxRenderer::EndFrame() {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->EndFrame();
  }
#endif
}

void TgfxRenderer::Execute(const RenderCommand& command) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ == nullptr) {
    return;
  }
  auto* impl = static_cast<GlRendererImpl*>(impl_);
  switch (command.type) {
    case RenderCommandType::kDrawRect:
      impl->DrawRect(command.rect, command.color);
      break;
    case RenderCommandType::kDrawRoundedRect:
      impl->DrawRoundedRect(command.rect, command.color,
                            command.corner_radius);
      break;
    case RenderCommandType::kDrawText:
      impl->DrawText(command.text, command.point, command.color,
                     command.font_size, command.font_name);
      break;
    case RenderCommandType::kDrawTexture:
      impl->DrawTexture(command.texture_id, command.rect);
      break;
    case RenderCommandType::kSave:
      impl->Save();
      break;
    case RenderCommandType::kRestore:
      impl->Restore();
      break;
    case RenderCommandType::kTranslate:
      impl->Translate(command.translate_x, command.translate_y);
      break;
    case RenderCommandType::kClipRect:
      impl->ClipRect(command.rect);
      break;
    default:
      break;
  }
#else
  (void)command;
#endif
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width;
  height_ = height;
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->Resize(width, height);
  }
#endif
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }

int TgfxRenderer::GetHeight() const noexcept { return height_; }

}  // namespace neoflux

