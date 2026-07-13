#pragma once

#include "neoflux/core/BuildContext.h"
#include "neoflux/core/Widget.h"
#include "neoflux/render/RenderContext.h"
#include "neoflux/render/Renderer.h"
#include "neoflux/util/SPSCQueue.h"

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace neoflux {
namespace core {
class Widget;
}
namespace engine {

enum class RenderBackend {
  kSkia,
  kSoftware,
  kNull,
};

enum class SkiaGraphicsApi {
  kOpenGL,
  kVulkan,
  kSoftware,
};

struct RenderPipelineConfig {
  RenderBackend backend = RenderBackend::kSkia;
  SkiaGraphicsApi skiaApi = SkiaGraphicsApi::kOpenGL;
  bool antialiasing = true;
  bool enableCaching = true;
  bool useGpu = false;
  std::string_view platform = "unknown";
  std::string_view target = "default";
};

std::string_view platformName();
std::string_view renderBackendName(RenderBackend backend);
std::string_view skiaApiName(SkiaGraphicsApi api);

class RenderEngine {
 public:
  explicit RenderEngine(RenderPipelineConfig config = {});
  ~RenderEngine();

  bool initialize(int width, int height);
  void setPipelineConfig(RenderPipelineConfig config);
  const RenderPipelineConfig& pipelineConfig() const;
  bool renderWidget(std::shared_ptr<core::Widget> root, const core::BuildContext& context);
  bool renderToFile(const core::Widget& root, const std::string& outputPath, int width,
                    int height) const;
  std::string buildTerminalPreview(std::string_view title, std::string_view body,
                                   std::string_view action) const;

 private:
  void renderLoop();

  RenderPipelineConfig pipelineConfig_;
  struct RenderRequest {
    std::shared_ptr<core::Widget> root;
    core::BuildContext context;
  };
  util::SPSCQueue<RenderRequest> requestQueue_;
  std::unique_ptr<render::Renderer> renderer_;
  std::thread renderThread_;
  std::atomic<bool> running_{false};
  ::neoflux::render::RenderContext renderContext_;
};

} // namespace engine
} // namespace neoflux
