#include "neoflux/engine/RenderEngine.h"
#include "neoflux/render/NullRenderer.h"
#include "neoflux/render/SkiaGlfwRenderer.h"

#include <glog/logging.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>

#include "taitank.h"

namespace neoflux {
namespace engine {

namespace {
constexpr std::string_view kDefaultPlatform = "linux";
}  // namespace

std::string_view platformName() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#else
  return kDefaultPlatform;
#endif
}

std::string_view renderBackendName(RenderBackend backend) {
  switch (backend) {
    case RenderBackend::kSkia:
      return "skia";
    case RenderBackend::kSoftware:
      return "software";
    case RenderBackend::kNull:
      return "null";
  }
  return "unknown";
}

std::string_view skiaApiName(SkiaGraphicsApi api) {
  switch (api) {
    case SkiaGraphicsApi::kOpenGL:
      return "opengl";
    case SkiaGraphicsApi::kVulkan:
      return "vulkan";
    case SkiaGraphicsApi::kSoftware:
      return "software";
  }
  return "unknown";
}

RenderEngine::RenderEngine(RenderPipelineConfig config)
    : pipelineConfig_(std::move(config)), requestQueue_(1024), running_(false) {}

RenderEngine::~RenderEngine() {
  running_.store(false);
  if (renderThread_.joinable()) {
    renderThread_.join();
  }
}

bool RenderEngine::initialize(int width, int height) {
  if (running_.load()) {
    return true;
  }

  renderContext_.width = width;
  renderContext_.height = height;

  switch (pipelineConfig_.backend) {
    case RenderBackend::kSkia:
      renderer_ = std::make_unique<render::SkiaGlfwRenderer>(pipelineConfig_);
      break;
    case RenderBackend::kNull:
      renderer_ = std::make_unique<render::NullRenderer>();
      break;
    case RenderBackend::kSoftware:
      renderer_ = std::make_unique<render::NullRenderer>();
      break;
  }

  if (!renderer_ || !renderer_->initialize(width, height)) {
    LOG(ERROR) << "RenderEngine: failed to initialize renderer";
    return false;
  }

  running_.store(true);
  renderThread_ = std::thread(&RenderEngine::renderLoop, this);
  return true;
}

void RenderEngine::setPipelineConfig(RenderPipelineConfig config) {
  pipelineConfig_ = std::move(config);
}

const RenderPipelineConfig& RenderEngine::pipelineConfig() const {
  return pipelineConfig_;
}

bool RenderEngine::renderWidget(std::shared_ptr<core::Widget> root,
                                const core::BuildContext& context) {
  if (!running_.load() || !renderer_) {
    LOG(WARNING) << "RenderEngine: render thread not initialized.";
    return false;
  }

  RenderRequest request;
  request.root = std::move(root);
  request.context = context;
  while (!requestQueue_.try_push(std::move(request))) {
    if (!running_.load()) {
      return false;
    }
    std::this_thread::yield();
  }
  return true;
}

void RenderEngine::renderLoop() {
  while (running_.load() && renderer_ && renderer_->isRunning()) {
    RenderRequest request;
    if (requestQueue_.try_pop(request)) {
      renderer_->renderScene(request.root, request.context);
    }
    renderer_->pollEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
  }
}

namespace {

void writePlaceholderPpm(std::ofstream& out, int width, int height) {
  out << "P6\n" << width << ' ' << height << "\n255\n";

  const int safeWidth = std::max(1, width);
  const int safeHeight = std::max(1, height);
  for (int y = 0; y < safeHeight; ++y) {
    for (int x = 0; x < safeWidth; ++x) {
      const uint8_t r = static_cast<uint8_t>((x * 255) / (safeWidth - 1));
      const uint8_t g = static_cast<uint8_t>((y * 255) / (safeHeight - 1));
      const uint8_t b = static_cast<uint8_t>(128 + ((x + y) % 64) * 2);
      out.put(r);
      out.put(g);
      out.put(b);
    }
  }
}

}  // namespace

bool RenderEngine::renderToFile(const core::Widget& root, const std::string& outputPath, int width,
                                int height) const {
  if (outputPath.empty() || width <= 0 || height <= 0) {
    return false;
  }

  std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }

  (void)root;
  writePlaceholderPpm(out, width, height);
  return out.good();
}

std::string RenderEngine::buildTerminalPreview(std::string_view title, std::string_view body,
                                               std::string_view action) const {
  using taitank::DoLayout;
  using taitank::GetHeight;
  using taitank::GetWidth;
  using taitank::InsertChild;
  using taitank::NodeCreate;
  using taitank::SetAlignItems;
  using taitank::SetFlexDirection;
  using taitank::SetHeight;
  using taitank::SetJustifyContent;
  using taitank::SetWidth;

  auto root = NodeCreate();
  SetWidth(root, 60.0f);
  SetHeight(root, 12.0f);
  SetFlexDirection(root, taitank::FLEX_DIRECTION_COLUMN);
  SetJustifyContent(root, taitank::FLEX_ALIGN_CENTER);
  SetAlignItems(root, taitank::FLEX_ALIGN_CENTER);

  auto card = NodeCreate();
  SetWidth(card, 56.0f);
  SetHeight(card, 9.0f);
  SetFlexDirection(card, taitank::FLEX_DIRECTION_COLUMN);
  SetJustifyContent(card, taitank::FLEX_ALIGN_START);
  SetAlignItems(card, taitank::FLEX_ALIGN_START);
  InsertChild(root, card, 0);

  auto header = NodeCreate();
  SetWidth(header, 52.0f);
  SetHeight(header, 1.0f);
  InsertChild(card, header, 0);

  auto bodyNode = NodeCreate();
  SetWidth(bodyNode, 52.0f);
  SetHeight(bodyNode, 1.0f);
  InsertChild(card, bodyNode, 1);

  auto actionNode = NodeCreate();
  SetWidth(actionNode, 20.0f);
  SetHeight(actionNode, 1.0f);
  InsertChild(card, actionNode, 2);

  DoLayout(root, 60.0f, 12.0f);

  const int width = std::max(24, static_cast<int>(GetWidth(card)) + 4);
  const int height = std::max(6, static_cast<int>(GetHeight(card)) + 2);
  std::vector<std::string> lines(height, std::string(width, ' '));

  const auto drawText = [&](int row, int col, std::string_view text) {
    if (row < 0 || row >= height) {
      return;
    }
    const int start = std::clamp(col, 0, width - 1);
    for (size_t index = 0; index < text.size() && start + static_cast<int>(index) < width; ++index) {
      lines[row][start + static_cast<int>(index)] = text[index];
    }
  };

  const std::string_view kTopLeft = "+";
  const std::string_view kTopRight = "+";
  const std::string_view kBottomLeft = "+";
  const std::string_view kBottomRight = "+";
  const std::string_view kHorizontal = "-";
  const std::string_view kVertical = "|";

  drawText(0, 0, kTopLeft);
  for (int col = 1; col < width - 1; ++col) {
    drawText(0, col, kHorizontal);
  }
  drawText(0, width - 1, kTopRight);
  drawText(height - 1, 0, kBottomLeft);
  for (int col = 1; col < width - 1; ++col) {
    drawText(height - 1, col, kHorizontal);
  }
  drawText(height - 1, width - 1, kBottomRight);
  for (int row = 1; row < height - 1; ++row) {
    drawText(row, 0, kVertical);
    drawText(row, width - 1, kVertical);
  }

  std::string pipelineSummary = std::string{renderBackendName(pipelineConfig_.backend)} + "/" +
                                std::string{skiaApiName(pipelineConfig_.skiaApi)} + " on " +
                                std::string{pipelineConfig_.platform};
  drawText(1, 2, "NeoFlux UI");
  drawText(1, width - 2 - static_cast<int>(pipelineSummary.size()), pipelineSummary);
  drawText(2, 2, std::string{title});
  drawText(3, 2, std::string{body});
  drawText(4, 2, "[" + std::string{action} + "]");
  drawText(5, 2, "flex layout: taitank");
  drawText(6, 2, "backend=" + std::string{renderBackendName(pipelineConfig_.backend)} +
                      " skia-api=" + std::string{skiaApiName(pipelineConfig_.skiaApi)});

  std::string preview;
  for (const auto& line : lines) {
    preview += line + '\n';
  }
  return preview;
}

} // namespace engine
} // namespace neoflux
