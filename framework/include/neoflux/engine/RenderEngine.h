#pragma once

#include <string_view>

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

  void setPipelineConfig(RenderPipelineConfig config);
  const RenderPipelineConfig& pipelineConfig() const;
  void renderWidget(const core::Widget& root) const;

 private:
  RenderPipelineConfig pipelineConfig_;
};

} // namespace engine
} // namespace neoflux
