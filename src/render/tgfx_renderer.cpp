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
#include <cstdint>
#include <cstring>
#include <cctype>
#include <filesystem>
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

#ifdef NEOFLUX_PLATFORM_DESKTOP
#include <GLFW/glfw3.h>
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

// ---------------------------------------------------------------------------
// OpenGL function loader. Loads all required entry points via
// glfwGetProcAddress after a context is current.
// ---------------------------------------------------------------------------
struct GlLoader {
  void(APIENTRY* glEnable)(GlEnum) = nullptr;
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
  void(APIENTRY* glDeleteTextures)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteBuffers)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteVertexArrays)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* glDeleteProgram)(GlUint) = nullptr;

  bool Load() {
// NOLINTBEGIN(bugprone-macro-parentheses)
#define NEOFLUX_LOAD_GL(name)                                       \
  name = reinterpret_cast<decltype(name)>(glfwGetProcAddress(#name)); \
  if (name == nullptr) {                                            \
    LOG(ERROR) << "Failed to load " << #name;                       \
    return false;                                                   \
  }
  // NOLINTEND(bugprone-macro-parentheses)
  // NOLINTBEGIN(bugprone-macro-parentheses)
  NEOFLUX_LOAD_GL(glEnable)
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
    NEOFLUX_LOAD_GL(glDrawArrays)
    NEOFLUX_LOAD_GL(glGenTextures)
    NEOFLUX_LOAD_GL(glBindTexture)
    NEOFLUX_LOAD_GL(glTexParameteri)
    NEOFLUX_LOAD_GL(glTexImage2D)
    NEOFLUX_LOAD_GL(glTexSubImage2D)
    NEOFLUX_LOAD_GL(glActiveTexture)
    NEOFLUX_LOAD_GL(glPixelStorei)
    NEOFLUX_LOAD_GL(glDeleteTextures)
    NEOFLUX_LOAD_GL(glDeleteBuffers)
    NEOFLUX_LOAD_GL(glDeleteVertexArrays)
    NEOFLUX_LOAD_GL(glDeleteProgram)
  // NOLINTEND(bugprone-macro-parentheses)
#undef NEOFLUX_LOAD_GL
    return true;
  }
};

// Vertex format: position (x, y) + texcoord (u, v), 4 floats per vertex.
constexpr int kVertexSize = 4;

// Texture atlas dimensions for glyph caching.
constexpr int kAtlasSize = 1024;

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

  bool Init(int width, int height, void* native_handle) {
    window_ = static_cast<GLFWwindow*>(native_handle);
    width_ = width;
    height_ = height;
    // GL initialization is deferred to BeginFrame, which runs on the render
    // thread where the GL context is current. FreeType does not need GL.
    InitFonts();
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
    gl.glClearColor(clear.r / 255.0F, clear.g / 255.0F, clear.b / 255.0F,
                    clear.a / 255.0F);
    gl.glClear(0x00004000U | 0x00000100U);
    transform_stack_.clear();
    transform_stack_.push_back({0.0F, 0.0F});
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

    gl.glUseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r / 255.0F, color.g / 255.0F,
                   color.b / 255.0F, color.a / 255.0F);
    gl.glUniform1i(u_use_texture_, 0);
    gl.glBindVertexArray(vao_);
    gl.glBindBuffer(0x8892, vbo_);
    gl.glBufferData(0x8892, sizeof(vertices), vertices, 0x88E8);
    gl.glDrawArrays(0x0004, 0, 6);
  }

  void DrawText(std::string_view text, const Point& position,
                const Color& color, float font_size,
                std::string_view font_name) {
    if (!gl_ready_) {
      return;
    }
    FT_Face face = GetFontFace(font_name);
    if (face == nullptr) {
      return;
    }
    const Transform& t = transform_stack_.back();
    float cursor_x = position.x;
    const float baseline_y = position.y;

    gl.glUseProgram(program_);
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r / 255.0F, color.g / 255.0F,
                   color.b / 255.0F, color.a / 255.0F);
    gl.glUniform1i(u_use_texture_, 1);
    gl.glActiveTexture(0x84C0);
    gl.glBindTexture(0x0DE1, atlas_texture_);
    gl.glBindVertexArray(vao_);
    gl.glBindBuffer(0x8892, vbo_);

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

      gl.glBufferData(0x8892, sizeof(vertices), vertices, 0x88E8);
      gl.glDrawArrays(0x0004, 0, 6);

      cursor_x += glyph->advance;
    }
  }

  void Save() {
    transform_stack_.push_back(transform_stack_.empty()
                                   ? Transform{0.0F, 0.0F}
                                   : transform_stack_.back());
  }

  void Restore() {
    if (transform_stack_.size() > 1) {
      transform_stack_.pop_back();
    }
  }

  void Translate(float delta_x, float delta_y) {
    if (!transform_stack_.empty()) {
      transform_stack_.back().x += delta_x;
      transform_stack_.back().y += delta_y;
    }
  }

  void ClipRect(const Rect& /*rect*/) {}

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

    static const char* kVertSrc = R"(
      #version 330 core
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
    static const char* kFragSrc = R"(
      #version 330 core
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

    GlUint vs = gl.glCreateShader(0x8B31);
    GlUint fs = gl.glCreateShader(0x8B30);
    gl.glShaderSource(vs, 1, &kVertSrc, nullptr);
    gl.glShaderSource(fs, 1, &kFragSrc, nullptr);
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

  // Initializes FreeType and scans the default font directories.
  void InitFonts() {
    if (FT_Init_FreeType(&ft_library_) != 0) {
      LOG(ERROR) << "Failed to initialize FreeType";
      return;
    }
    // Scan common font locations. Developers place their fonts in
    // thirdparty/fonts/ and reference them by filename stem.
    font_manager_.ScanDirectory("thirdparty/fonts");
    font_manager_.ScanDirectory("../thirdparty/fonts");
    font_manager_.ScanDirectory("../../thirdparty/fonts");
    if (font_manager_.GetFontCount() == 0) {
      LOG(WARNING) << "No fonts found in thirdparty/fonts/. "
                   << "Text rendering will be disabled.";
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
    auto it = glyph_cache_.find(key);
    if (it != glyph_cache_.end()) {
      return &it->second;
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
      return &glyph_cache_[key];
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
        std::memcpy(packed.data() + static_cast<std::size_t>(row) * w,
                    bitmap.buffer + (h - 1 - row) * (-pitch),
                    static_cast<std::size_t>(w));
      }
    }

    gl.glBindTexture(0x0DE1, atlas_texture_);
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

    return &glyph_cache_[key];
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
  int atlas_x_ = 0;
  int atlas_y_ = 0;
  int atlas_row_height_ = 0;

  std::vector<Transform> transform_stack_{};
};

}  // namespace

#endif  // NEOFLUX_PLATFORM_DESKTOP

// =============================================================================
// TgfxRenderer public interface
// =============================================================================

TgfxRenderer::TgfxRenderer() = default;

TgfxRenderer::~TgfxRenderer() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  delete static_cast<GlRendererImpl*>(impl_);
#endif
}

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
  if (initialized_) {
    return true;
  }
#ifdef NEOFLUX_PLATFORM_DESKTOP
  auto* impl = new GlRendererImpl();
  if (!impl->Init(width, height, native_handle)) {
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
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->BeginFrame(clear_color);
  }
#else
  (void)clear_color;
#endif
}

void TgfxRenderer::EndFrame() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->EndFrame();
  }
#endif
}

void TgfxRenderer::Execute(const RenderCommand& command) {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (impl_ == nullptr) {
    return;
  }
  auto* impl = static_cast<GlRendererImpl*>(impl_);
  switch (command.type) {
    case RenderCommandType::kDrawRect:
      impl->DrawRect(command.rect, command.color);
      break;
    case RenderCommandType::kDrawText:
      impl->DrawText(command.text, command.point, command.color,
                     command.font_size, command.font_name);
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
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (impl_ != nullptr) {
    static_cast<GlRendererImpl*>(impl_)->Resize(width, height);
  }
#endif
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }

int TgfxRenderer::GetHeight() const noexcept { return height_; }

}  // namespace neoflux

