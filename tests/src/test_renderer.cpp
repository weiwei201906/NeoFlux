#include "gtest/gtest.h"

#include "neoflux/render/Renderer.h"
#include "neoflux/render/NullRenderer.h"
#include "neoflux/core/Widget.h"

#include <thread>

using namespace neoflux;

class DummyWidget : public core::Widget {
 public:
  DummyWidget() : core::Widget("dummy") {}
  void render(const render::RenderContext& ctx) const override {
    (void)ctx;
    // trivial work
  }
};

TEST(NullRendererTest, RunAndShutdown) {
  auto root = std::make_shared<DummyWidget>();
  render::NullRenderer r;
  ASSERT_TRUE(r.initialize(640, 480));
  std::thread t([&]() {
    neoflux::core::BuildContext ctx;
    ctx.width = 640;
    ctx.height = 480;
    while (r.isRunning()) {
      r.renderScene(root, ctx);
      r.pollEvents();
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  r.requestShutdown();
  t.join();
  SUCCEED();
}
