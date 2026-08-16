// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// Renderer backend. Two implementations are compiled conditionally:
//
//   - NEOFLUX_HAVE_TGFX: tgfx performs all rendering (rects, UTF-8 text,
//     clipping, transforms). A custom tgfx::Device/Window wraps the GLFW
//     OpenGL context and default framebuffer.
//
//   - Fallback (no tgfx): a modern OpenGL renderer using shaders + VBO for
//     geometry and stb_truetype for UTF-8 TrueType text. GLFW remains only
//     a window/context/input bridge.
//
// In both paths, GLFW only manages the window, GL context, input, and
// buffer swap. All UI drawing is handled by the renderer.
// =============================================================================

#include "neoflux/render/tgfx_renderer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

#ifdef NEOFLUX_HAVE_TGFX

// ---------------------------------------------------------------------------
// tgfx implementation
// ---------------------------------------------------------------------------
#include "tgfx/core/Canvas.h"
#include "tgfx/core/Color.h"
#include "tgfx/core/Font.h"
#include "tgfx/core/Paint.h"
#include "tgfx/core/Rect.h"
#include "tgfx/core/Surface.h"
#include "tgfx/core/Typeface.h"
#include "tgfx/gpu/Backend.h"
#include "tgfx/gpu/Context.h"
#include "tgfx/gpu/Device.h"
#include "tgfx/gpu/GPU.h"
#include "tgfx/gpu/Window.h"
#include "gpu/opengl/GLGPU.h"
#include "gpu/opengl/GLInterface.h"
#include "gpu/opengl/GLProcGetter.h"
#include "gpu/proxies/RenderTargetProxy.h"

namespace neoflux {
namespace {

class GLFWTgfxDevice final : public tgfx::Device {
 public:
  explicit GLFWTgfxDevice(GLFWwindow* window)
      : Device(CreateGpu(window)), window_(window) {}
  ~GLFWTgfxDevice() override = default;

 protected:
  bool onLockContext() override {
    glfwMakeContextCurrent(window_);
    return true;
  }
  void onUnlockContext() override {}

 private:
  static std::unique_ptr<tgfx::GPU> CreateGpu(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    auto getter = tgfx::GLProcGetter::Make();
    if (getter == nullptr) return nullptr;
    auto interface = tgfx::GLInterface::MakeNativeInterface(getter.get());
    if (interface == nullptr) return nullptr;
    return std::make_unique<tgfx::GLGPU>(std::move(interface));
  }
  GLFWwindow* window_ = nullptr;
};

class GLFWTgfxWindow final : public tgfx::Window {
 public:
  GLFWTgfxWindow(std::shared_ptr<tgfx::Device> device, GLFWwindow* window,
                 int w, int h)
      : tgfx::Window(std::move(device)), window_(window), width_(w), height_(h) {}
  void Resize(int w, int h) noexcept { width_ = w; height_ = h; }

 protected:
  std::shared_ptr<tgfx::RenderTargetProxy> onCreateRenderTarget(
      tgfx::Context* context) override {
    tgfx::GLFrameBufferInfo fb{};
    fb.id = 0;
    fb.format = 0x8058;  // GL_RGBA8
    tgfx::BackendRenderTarget rt(fb, width_, height_);
    return tgfx::RenderTargetProxy::MakeFrom(context, rt, 1,
                                             tgfx::ImageOrigin::BottomLeft);
  }
  void onPresent(tgfx::Context*) override { glfwSwapBuffers(window_); }

 private:
  GLFWwindow* window_ = nullptr;
  int width_ = 0;
  int height_ = 0;
};

tgfx::Color ToTgfxColor(const Color& c) noexcept {
  return {c.r / 255.0F, c.g / 255.0F, c.b / 255.0F, c.a / 255.0F};
}
tgfx::Rect ToTgfxRect(const Rect& r) noexcept {
  return tgfx::Rect::MakeLTRB(r.x, r.y, r.x + r.width, r.y + r.height);
}

}  // namespace

TgfxRenderer::TgfxRenderer() = default;
TgfxRenderer::~TgfxRenderer() {
  if (surface_) delete static_cast<std::shared_ptr<tgfx::Surface>*>(surface_);
  if (typeface_) delete static_cast<std::shared_ptr<tgfx::Typeface>*>(typeface_);
  if (window_) delete static_cast<GLFWTgfxWindow*>(window_);
  if (device_) delete static_cast<std::shared_ptr<tgfx::Device>*>(device_);
}

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
  if (initialized_) return true;
  if (!native_handle) return false;
  auto* win = static_cast<GLFWwindow*>(native_handle);
  auto dev = std::make_shared<GLFWTgfxDevice>(win);
  device_ = new std::shared_ptr<tgfx::Device>(dev);
  auto* tgfx_win = new GLFWTgfxWindow(dev, win, width, height);
  window_ = tgfx_win;
  auto surf = tgfx_win->getSurface();
  if (!surf) { LOG(ERROR) << "tgfx failed to create surface"; return false; }
  surface_ = new std::shared_ptr<tgfx::Surface>(surf);
  canvas_ = surf->getCanvas();
  auto tf = tgfx::Typeface::MakeFromPath("C:/Windows/Fonts/msyh.ttc", 0);
  if (!tf) tf = tgfx::Typeface::MakeFromPath("C:/Windows/Fonts/simhei.ttf", 0);
  typeface_ = new std::shared_ptr<tgfx::Typeface>(tf);
  width_ = width; height_ = height; initialized_ = true;
  return true;
}

void TgfxRenderer::BeginFrame(const Color& clear) {
  if (!surface_) return;
  auto* surf = *static_cast<std::shared_ptr<tgfx::Surface>*>(surface_);
  if (!surf) return;
  canvas_ = surf->getCanvas();
  if (canvas_) static_cast<tgfx::Canvas*>(canvas_)->clear(ToTgfxColor(clear));
}

void TgfxRenderer::EndFrame() {
  if (!surface_) return;
  auto* surf = *static_cast<std::shared_ptr<tgfx::Surface>*>(surface_);
  if (surf) surf->flushAndSubmit();
  if (window_) static_cast<GLFWTgfxWindow*>(window_)->present();
}

void TgfxRenderer::Execute(const RenderCommand& cmd) {
  if (!canvas_) return;
  auto* canvas = static_cast<tgfx::Canvas*>(canvas_);
  switch (cmd.type) {
    case RenderCommandType::kDrawRect: {
      tgfx::Paint paint;
      paint.setColor(ToTgfxColor(cmd.color));
      paint.setStyle(tgfx::Paint::Style::Fill);
      canvas->drawRect(ToTgfxRect(cmd.rect), paint);
      break;
    }
    case RenderCommandType::kDrawText: {
      tgfx::Font font;
      if (typeface_) font.setTypeface(*static_cast<std::shared_ptr<tgfx::Typeface>*>(typeface_));
      font.setSize(cmd.font_size);
      tgfx::Paint paint;
      paint.setColor(ToTgfxColor(cmd.color));
      canvas->drawSimpleText(cmd.text, cmd.point.x, cmd.point.y, font, paint);
      break;
    }
    case RenderCommandType::kSave: canvas->save(); break;
    case RenderCommandType::kRestore: canvas->restore(); break;
    case RenderCommandType::kTranslate: canvas->translate(cmd.translate_x, cmd.translate_y); break;
    case RenderCommandType::kClipRect: canvas->clipRect(ToTgfxRect(cmd.rect)); break;
    default: break;
  }
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width; height_ = height;
  if (window_) static_cast<GLFWTgfxWindow*>(window_)->Resize(width, height);
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }
int TgfxRenderer::GetHeight() const noexcept { return height_; }

void TgfxRenderer::DrawRectImpl(const Rect&, const Color&) {}
void TgfxRenderer::DrawTextImpl(std::string_view, const Point&, const Color&, float) {}
void TgfxRenderer::SaveImpl() {}
void TgfxRenderer::RestoreImpl() {}
void TgfxRenderer::TranslateImpl(float, float) {}
void TgfxRenderer::ClipRectImpl(const Rect&) {}

}  // namespace neoflux

#else  // !NEOFLUX_HAVE_TGFX — OpenGL fallback renderer

// ---------------------------------------------------------------------------
// OpenGL fallback: modern GL 3.3 core + stb_truetype for UTF-8 text.
// All GL initialization is deferred to the render thread (BeginFrame)
// because the GL context is only current there.
// ---------------------------------------------------------------------------

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace neoflux {
namespace {

// Minimal GL function loader (core 3.3 subset needed by the renderer).
struct GlLoader {
  using PFNGLCLEAR = void(*)(unsigned int);
  using PFNGLCLEARCOLOR = void(*)(float, float, float, float);
  using PFNGLVIEWPORT = void(*)(int, int, int, int);
  using PFNGLENABLE = void(*)(unsigned int);
  using PFNGLBLENDFUNC = void(*)(unsigned int, unsigned int);
  using PFNGLCREATESHADER = unsigned int(*)(unsigned int);
  using PFNGLSHADERSOURCE = void(*)(unsigned int, int, const char**, const int*);
  using PFNGLCOMPILESHADER = void(*)(unsigned int);
  using PFNGLGETSHADERIV = void(*)(unsigned int, unsigned int, int*);
  using PFNGLGETSHADERINFOLOG = void(*)(unsigned int, int, int*, char*);
  using PFNGLCREATEPROGRAM = unsigned int(*)();
  using PFNGLATTACHSHADER = void(*)(unsigned int, unsigned int);
  using PFNGLLINKPROGRAM = void(*)(unsigned int);
  using PFNGLGETPROGRAMIV = void(*)(unsigned int, unsigned int, int*);
  using PFNGLGETPROGRAMINFOLOG = void(*)(unsigned int, int, int*, char*);
  using PFNGLUSEPROGRAM = void(*)(unsigned int);
  using PFNGLGETUNIFORMLOCATION = int(*)(unsigned int, const char*);
  using PFNGLUNIFORM4F = void(*)(int, float, float, float, float);
  using PFNGLUNIFORM2F = void(*)(int, float, float);
  using PFNGLUNIFORM1I = void(*)(int, int);
  using PFNGLGENVERTEXARRAYS = void(*)(int, unsigned int*);
  using PFNGLBINDVERTEXARRAY = void(*)(unsigned int);
  using PFNGLGENBUFFERS = void(*)(int, unsigned int*);
  using PFNGLBINDBUFFER = void(*)(unsigned int, unsigned int);
  using PFNGLBUFFERDATA = void(*)(unsigned int, long long, const void*, unsigned int);
  using PFNGLENABLEVERTEXATTRIBARRAY = void(*)(unsigned int);
  using PFNGLVERTEXATTRIBPOINTER = void(*)(unsigned int, int, unsigned int, unsigned char, int, const void*);
  using PFNGLDRAWARRAYS = void(*)(unsigned int, int, int);
  using PFNGLGENTEXTURES = void(*)(int, unsigned int*);
  using PFNGLTEXIMAGE2D = void(*)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
  using PFNGLTEXPARAMETERI = void(*)(unsigned int, unsigned int, int);
  using PFNGLTEXPARAMETERIV = void(*)(unsigned int, unsigned int, const int*);
  using PFNGLACTIVETEXTURE = void(*)(unsigned int);
  using PFNGLBINDTEXTURE = void(*)(unsigned int, unsigned int);
  using PFNGLSCISSOR = void(*)(int, int, int, int);
  using PFNGLDISABLE = void(*)(unsigned int);

  PFNGLCLEAR glClear = nullptr;
  PFNGLCLEARCOLOR glClearColor = nullptr;
  PFNGLVIEWPORT glViewport = nullptr;
  PFNGLENABLE glEnable = nullptr;
  PFNGLDISABLE glDisable = nullptr;
  PFNGLBLENDFUNC glBlendFunc = nullptr;
  PFNGLCREATESHADER glCreateShader = nullptr;
  PFNGLSHADERSOURCE glShaderSource = nullptr;
  PFNGLCOMPILESHADER glCompileShader = nullptr;
  PFNGLGETSHADERIV glGetShaderiv = nullptr;
  PFNGLGETSHADERINFOLOG glGetShaderInfoLog = nullptr;
  PFNGLCREATEPROGRAM glCreateProgram = nullptr;
  PFNGLATTACHSHADER glAttachShader = nullptr;
  PFNGLLINKPROGRAM glLinkProgram = nullptr;
  PFNGLGETPROGRAMIV glGetProgramiv = nullptr;
  PFNGLGETPROGRAMINFOLOG glGetProgramInfoLog = nullptr;
  PFNGLUSEPROGRAM glUseProgram = nullptr;
  PFNGLGETUNIFORMLOCATION glGetUniformLocation = nullptr;
  PFNGLUNIFORM4F glUniform4f = nullptr;
  PFNGLUNIFORM2F glUniform2f = nullptr;
  PFNGLUNIFORM1I glUniform1i = nullptr;
  PFNGLGENVERTEXARRAYS glGenVertexArrays = nullptr;
  PFNGLBINDVERTEXARRAY glBindVertexArray = nullptr;
  PFNGLGENBUFFERS glGenBuffers = nullptr;
  PFNGLBINDBUFFER glBindBuffer = nullptr;
  PFNGLBUFFERDATA glBufferData = nullptr;
  PFNGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray = nullptr;
  PFNGLVERTEXATTRIBPOINTER glVertexAttribPointer = nullptr;
  PFNGLDRAWARRAYS glDrawArrays = nullptr;
  PFNGLGENTEXTURES glGenTextures = nullptr;
  PFNGLTEXIMAGE2D glTexImage2D = nullptr;
  PFNGLTEXPARAMETERI glTexParameteri = nullptr;
  PFNGLTEXPARAMETERIV glTexParameteriv = nullptr;
  PFNGLACTIVETEXTURE glActiveTexture = nullptr;
  PFNGLBINDTEXTURE glBindTexture = nullptr;
  PFNGLSCISSOR glScissor = nullptr;

  bool Load() {
#define LOAD(name) name = reinterpret_cast<decltype(name)>(glfwGetProcAddress(#name)); if (!name) return false
    LOAD(glClear); LOAD(glClearColor); LOAD(glViewport); LOAD(glEnable); LOAD(glDisable);
    LOAD(glBlendFunc); LOAD(glCreateShader); LOAD(glShaderSource); LOAD(glCompileShader);
    LOAD(glGetShaderiv); LOAD(glGetShaderInfoLog); LOAD(glCreateProgram); LOAD(glAttachShader);
    LOAD(glLinkProgram); LOAD(glGetProgramiv); LOAD(glGetProgramInfoLog); LOAD(glUseProgram);
    LOAD(glGetUniformLocation); LOAD(glUniform4f); LOAD(glUniform2f); LOAD(glUniform1i);
    LOAD(glGenVertexArrays); LOAD(glBindVertexArray); LOAD(glGenBuffers); LOAD(glBindBuffer);
    LOAD(glBufferData); LOAD(glEnableVertexAttribArray); LOAD(glVertexAttribPointer);
    LOAD(glDrawArrays); LOAD(glGenTextures); LOAD(glTexImage2D); LOAD(glTexParameteri);
    LOAD(glActiveTexture); LOAD(glBindTexture); LOAD(glScissor); LOAD(glTexParameteriv);
#undef LOAD
    return true;
  }
};

struct GlyphInfo {
  unsigned int texture = 0;
  int width = 0;
  int height = 0;
  int x_offset = 0;
  int y_offset = 0;
  int advance = 0;
};

struct TransformState {
  float x = 0.0F;
  float y = 0.0F;
};

// Full GL renderer implementation. All GL calls happen on the render thread
// after the context is made current by RenderLayer::RenderLoop.
class GlRendererImpl {
 public:
  GlRendererImpl() = default;
  ~GlRendererImpl() {
    if (font_data_) delete[] font_data_;
  }

  // Called on main thread: just stores the window handle. No GL calls.
  void Prepare(GLFWwindow* win, int w, int h) {
    window_ = win; width_ = w; height_ = h;
  }

  // Called on render thread (context already current). Performs all GL setup.
  bool InitializeGL() {
    if (gl_ready_) return true;
    if (!gl.Load()) { LOG(ERROR) << "Failed to load GL functions"; return false; }
    if (!CreateShaderProgram()) { LOG(ERROR) << "Failed to create shader program"; return false; }
    gl.glGenVertexArrays(1, &vao_);
    gl.glGenBuffers(1, &vbo_);
    gl.glBindVertexArray(vao_);
    gl.glBindBuffer(0x8892, vbo_);  // GL_ARRAY_BUFFER
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 4, 0x1406, 0, 4 * sizeof(float), nullptr);  // GL_FLOAT
    u_res_ = gl.glGetUniformLocation(program_, "u_resolution");
    u_translate_ = gl.glGetUniformLocation(program_, "u_translate");
    u_color_ = gl.glGetUniformLocation(program_, "u_color");
    u_tex_ = gl.glGetUniformLocation(program_, "u_texture");
    u_use_tex_ = gl.glGetUniformLocation(program_, "u_use_texture");
    gl.glEnable(0x0BE2);  // GL_BLEND
    gl.glBlendFunc(0x0302, 0x0303);  // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
    LoadFont();
    gl_ready_ = true;
    LOG(INFO) << "OpenGL fallback renderer initialized (" << width_ << "x" << height_ << ")";
    return true;
  }

  void BeginFrame(const Color& clear) {
    if (!InitializeGL()) return;
    gl.glViewport(0, 0, width_, height_);
    gl.glClearColor(clear.r / 255.0F, clear.g / 255.0F, clear.b / 255.0F, clear.a / 255.0F);
    gl.glClear(0x00004000 | 0x00000100);  // GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
    transform_stack_.clear();
    transform_stack_.push_back({0.0F, 0.0F});
  }

  void EndFrame() { /* swap handled by GLFW bridge */ }

  void DrawRect(const Rect& rect, const Color& color) {
    if (!gl_ready_) return;
    const auto& t = transform_stack_.back();
    gl.glUseProgram(program_);
    gl.glUniform2f(u_res_, static_cast<float>(width_), static_cast<float>(height_));
    gl.glUniform2f(u_translate_, t.x, t.y);
    gl.glUniform4f(u_color_, color.r / 255.0F, color.g / 255.0F, color.b / 255.0F, color.a / 255.0F);
    gl.glUniform1i(u_use_tex_, 0);
    float vertices[] = {
      rect.x, rect.y, 0.0F, 1.0F,
      rect.x + rect.width, rect.y, 0.0F, 1.0F,
      rect.x, rect.y + rect.height, 0.0F, 1.0F,
      rect.x + rect.width, rect.y, 0.0F, 1.0F,
      rect.x + rect.width, rect.y + rect.height, 0.0F, 1.0F,
      rect.x, rect.y + rect.height, 0.0F, 1.0F,
    };
    gl.glBindVertexArray(vao_);
    gl.glBindBuffer(0x8892, vbo_);
    gl.glBufferData(0x8892, sizeof(vertices), vertices, 0x88E8);  // GL_STREAM_DRAW
    gl.glDrawArrays(0x0004, 0, 6);  // GL_TRIANGLES
  }

  void DrawText(std::string_view text, const Point& pos, const Color& color, float font_size) {
    if (!gl_ready_ || !font_loaded_) return;
    const auto& t = transform_stack_.back();
    float cursor_x = pos.x;
    const float baseline_y = pos.y;
    const int pixel_size = static_cast<int>(font_size + 0.5F);
    const float scale = stbtt_ScaleForPixelHeight(&font_, static_cast<float>(pixel_size));
    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&font_, &ascent, &descent, &line_gap);

    for (std::size_t i = 0; i < text.size();) {
      std::uint32_t cp = 0;
      unsigned char c = static_cast<unsigned char>(text[i]);
      if (c < 0x80) { cp = c; i += 1; }
      else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
        cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3F); i += 2;
      } else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
        cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6)
             | (static_cast<unsigned char>(text[i + 2]) & 0x3F); i += 3;
      } else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
        cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12)
             | ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6)
             | (static_cast<unsigned char>(text[i + 3]) & 0x3F); i += 4;
      } else { cp = c; i += 1; }

      const GlyphInfo& g = GetGlyph(cp, pixel_size);
      if (g.texture == 0) { cursor_x += static_cast<float>(g.advance) * scale; continue; }

      // stbtt_GetGlyphBitmap returns xoff/yoff in screen coordinates (y down).
      // bitmap position = (baseline_x + xoff, baseline_y + yoff).
      const float draw_x = cursor_x + static_cast<float>(g.x_offset);
      const float draw_y = baseline_y + static_cast<float>(g.y_offset);
      const float w = static_cast<float>(g.width);
      const float h = static_cast<float>(g.height);

      gl.glUseProgram(program_);
      gl.glUniform2f(u_res_, static_cast<float>(width_), static_cast<float>(height_));
      gl.glUniform2f(u_translate_, t.x + draw_x, t.y + draw_y);
      gl.glUniform4f(u_color_, color.r / 255.0F, color.g / 255.0F, color.b / 255.0F, color.a / 255.0F);
      gl.glUniform1i(u_use_tex_, 1);
      gl.glActiveTexture(0x84C0);  // GL_TEXTURE0
      gl.glBindTexture(0x0DE1, g.texture);  // GL_TEXTURE_2D
      gl.glUniform1i(u_tex_, 0);

      float verts[] = {
        0, 0, 0, 0,
        w, 0, 1, 0,
        0, h, 0, 1,
        w, 0, 1, 0,
        w, h, 1, 1,
        0, h, 0, 1,
      };
      gl.glBindVertexArray(vao_);
      gl.glBindBuffer(0x8892, vbo_);
      gl.glBufferData(0x8892, sizeof(verts), verts, 0x88E8);
      gl.glDrawArrays(0x0004, 0, 6);

      cursor_x += static_cast<float>(g.advance) * scale;
    }
  }

  void Save() { transform_stack_.push_back(transform_stack_.back()); }
  void Restore() { if (transform_stack_.size() > 1) transform_stack_.pop_back(); }
  void Translate(float dx, float dy) { transform_stack_.back().x += dx; transform_stack_.back().y += dy; }
  void ClipRect(const Rect&) { /* scissor clip not implemented in fallback */ }
  void Resize(int w, int h) { width_ = w; height_ = h; }

 private:
  bool CreateShaderProgram() {
    static const char* kVertSrc = R"(
      #version 330 core
      layout(location=0) in vec4 a_pos;
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
          float a = texture(u_texture, v_uv).a;
          frag_color = vec4(u_color.rgb, u_color.a * a);
        } else {
          frag_color = u_color;
        }
      })";
    unsigned int vs = gl.glCreateShader(0x8B31);  // GL_VERTEX_SHADER
    unsigned int fs = gl.glCreateShader(0x8B30);  // GL_FRAGMENT_SHADER
    gl.glShaderSource(vs, 1, &kVertSrc, nullptr);
    gl.glShaderSource(fs, 1, &kFragSrc, nullptr);
    gl.glCompileShader(vs);
    gl.glCompileShader(fs);
    int ok = 0;
    gl.glGetShaderiv(vs, 0x8B81, &ok);  // GL_COMPILE_STATUS
    if (!ok) { char log[512]; gl.glGetShaderInfoLog(vs, 512, nullptr, log); LOG(ERROR) << "VS compile: " << log; return false; }
    gl.glGetShaderiv(fs, 0x8B81, &ok);
    if (!ok) { char log[512]; gl.glGetShaderInfoLog(fs, 512, nullptr, log); LOG(ERROR) << "FS compile: " << log; return false; }
    program_ = gl.glCreateProgram();
    gl.glAttachShader(program_, vs);
    gl.glAttachShader(program_, fs);
    gl.glLinkProgram(program_);
    gl.glGetProgramiv(program_, 0x8B82, &ok);  // GL_LINK_STATUS
    if (!ok) { char log[512]; gl.glGetProgramInfoLog(program_, 512, nullptr, log); LOG(ERROR) << "Link: " << log; return false; }
    return true;
  }

  void LoadFont() {
    static const char* kFontPaths[] = {
      "C:/Windows/Fonts/msyh.ttc",
      "C:/Windows/Fonts/simhei.ttf",
      "C:/Windows/Fonts/segoeui.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/System/Library/Fonts/PingFang.ttc",
    };
    for (const char* path : kFontPaths) {
      FILE* f = fopen(path, "rb");
      if (!f) continue;
      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      fseek(f, 0, SEEK_SET);
      if (size <= 0) { fclose(f); continue; }
      font_data_ = new unsigned char[static_cast<std::size_t>(size)];
      if (fread(font_data_, 1, static_cast<std::size_t>(size), f) != static_cast<std::size_t>(size)) {
        fclose(f); delete[] font_data_; font_data_ = nullptr; continue;
      }
      fclose(f);
      if (stbtt_InitFont(&font_, font_data_, stbtt_GetFontOffsetForIndex(font_data_, 0))) {
        font_loaded_ = true;
        LOG(INFO) << "Loaded font: " << path;
        return;
      }
      delete[] font_data_; font_data_ = nullptr;
    }
    LOG(WARNING) << "No TrueType font found, text rendering disabled";
  }

  const GlyphInfo& GetGlyph(std::uint32_t codepoint, int pixel_size) {
    static GlyphInfo kEmpty{};
    auto key = std::make_pair(codepoint, pixel_size);
    auto it = glyph_cache_.find(key);
    if (it != glyph_cache_.end()) return it->second;
    int glyph_index = stbtt_FindGlyphIndex(&font_, static_cast<int>(codepoint));
    if (glyph_index == 0) { glyph_cache_[key] = kEmpty; return glyph_cache_[key]; }
    const float scale = stbtt_ScaleForPixelHeight(&font_, static_cast<float>(pixel_size));
    int w = 0, h = 0, xoff = 0, yoff = 0;
    unsigned char* bitmap = stbtt_GetGlyphBitmap(&font_, scale, scale, glyph_index, &w, &h, &xoff, &yoff);
    if (!bitmap || w == 0 || h == 0) {
      if (bitmap) stbtt_FreeBitmap(bitmap, nullptr);
      int advance = 0;
      stbtt_GetGlyphHMetrics(&font_, glyph_index, &advance, nullptr);
      GlyphInfo g; g.advance = advance;
      glyph_cache_[key] = g;
      return glyph_cache_[key];
    }
    unsigned int tex = 0;
    gl.glGenTextures(1, &tex);
    gl.glBindTexture(0x0DE1, tex);  // GL_TEXTURE_2D
    // GL_R8 sized internal format + GL_RED format. Swizzle R->A so the
    // shader can sample .a for the glyph coverage.
    gl.glTexImage2D(0x0DE1, 0, 0x8229, w, h, 0, 0x1903, 0x1401, bitmap);  // GL_R8, GL_RED, GL_UNSIGNED_BYTE
    int swizzle[] = {0, 0, 0, 0x1903};  // GL_ZERO, GL_ZERO, GL_ZERO, GL_RED
    gl.glTexParameteriv(0x0DE1, 0x8E46, swizzle);  // GL_TEXTURE_SWIZZLE_RGBA
    gl.glTexParameteri(0x0DE1, 0x2803, 0x2601);  // GL_TEXTURE_MIN_FILTER, GL_LINEAR
    gl.glTexParameteri(0x0DE1, 0x2802, 0x2601);  // GL_TEXTURE_MAG_FILTER, GL_LINEAR
    stbtt_FreeBitmap(bitmap, nullptr);
    int advance = 0;
    stbtt_GetGlyphHMetrics(&font_, glyph_index, &advance, nullptr);
    GlyphInfo g;
    g.texture = tex; g.width = w; g.height = h; g.x_offset = xoff; g.y_offset = yoff; g.advance = advance;
    glyph_cache_[key] = g;
    return glyph_cache_[key];
  }

  struct PairHash {
    std::size_t operator()(const std::pair<std::uint32_t, int>& p) const noexcept {
      return std::hash<std::uint64_t>{}(static_cast<std::uint64_t>(p.first) << 32 | static_cast<std::uint32_t>(p.second));
    }
  };

  GLFWwindow* window_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  bool gl_ready_ = false;
  GlLoader gl;
  unsigned int program_ = 0;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  int u_res_ = -1;
  int u_translate_ = -1;
  int u_color_ = -1;
  int u_tex_ = -1;
  int u_use_tex_ = -1;
  stbtt_fontinfo font_{};
  unsigned char* font_data_ = nullptr;
  bool font_loaded_ = false;
  std::unordered_map<std::pair<std::uint32_t, int>, GlyphInfo, PairHash> glyph_cache_;
  std::vector<TransformState> transform_stack_;
};

}  // namespace

// Pimpl wrapper: TgfxRenderer holds a GlRendererImpl when tgfx is absent.
TgfxRenderer::TgfxRenderer() = default;
TgfxRenderer::~TgfxRenderer() {
  if (impl_) delete static_cast<GlRendererImpl*>(impl_);
}

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
  if (initialized_) return true;
  if (!native_handle) return false;
  auto* impl = new GlRendererImpl();
  impl->Prepare(static_cast<GLFWwindow*>(native_handle), width, height);
  impl_ = impl;
  width_ = width; height_ = height; initialized_ = true;
  return true;
}

void TgfxRenderer::BeginFrame(const Color& clear) {
  if (impl_) static_cast<GlRendererImpl*>(impl_)->BeginFrame(clear);
}
void TgfxRenderer::EndFrame() {
  if (impl_) static_cast<GlRendererImpl*>(impl_)->EndFrame();
}

void TgfxRenderer::Execute(const RenderCommand& cmd) {
  if (!impl_) return;
  auto* impl = static_cast<GlRendererImpl*>(impl_);
  switch (cmd.type) {
    case RenderCommandType::kDrawRect: impl->DrawRect(cmd.rect, cmd.color); break;
    case RenderCommandType::kDrawText: impl->DrawText(cmd.text, cmd.point, cmd.color, cmd.font_size); break;
    case RenderCommandType::kSave: impl->Save(); break;
    case RenderCommandType::kRestore: impl->Restore(); break;
    case RenderCommandType::kTranslate: impl->Translate(cmd.translate_x, cmd.translate_y); break;
    case RenderCommandType::kClipRect: impl->ClipRect(cmd.rect); break;
    default: break;
  }
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width; height_ = height;
  if (impl_) static_cast<GlRendererImpl*>(impl_)->Resize(width, height);
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }
int TgfxRenderer::GetHeight() const noexcept { return height_; }

void TgfxRenderer::DrawRectImpl(const Rect&, const Color&) {}
void TgfxRenderer::DrawTextImpl(std::string_view, const Point&, const Color&, float) {}
void TgfxRenderer::SaveImpl() {}
void TgfxRenderer::RestoreImpl() {}
void TgfxRenderer::TranslateImpl(float, float) {}
void TgfxRenderer::ClipRectImpl(const Rect&) {}

}  // namespace neoflux

#endif  // NEOFLUX_HAVE_TGFX
