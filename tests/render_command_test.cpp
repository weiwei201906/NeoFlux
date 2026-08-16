// =============================================================================
// NeoFlux - render_command_test.cpp
//
// Unit tests for render command creation and the render context.
// =============================================================================

#include <neoflux/render/render_command.h>
#include <neoflux/render/render_context.h>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

TEST(RenderCommandTest, MakeDrawRect) {
  Rect rect{10.0F, 20.0F, 100.0F, 50.0F};
  Color color{255, 0, 0, 255};
  auto cmd = RenderCommand::MakeDrawRect(rect, color);

  EXPECT_EQ(cmd.type, RenderCommandType::kDrawRect);
  const auto& payload = std::get<DrawRectCommand>(cmd.payload);
  EXPECT_FLOAT_EQ(payload.rect.x, 10.0F);
  EXPECT_FLOAT_EQ(payload.rect.width, 100.0F);
  EXPECT_EQ(payload.color.r, 255);
}

TEST(RenderCommandTest, MakeDrawText) {
  auto cmd =
      RenderCommand::MakeDrawText("Hello", {5.0F, 10.0F}, {0, 0, 0, 255}, 16.0F);

  EXPECT_EQ(cmd.type, RenderCommandType::kDrawText);
  const auto& payload = std::get<DrawTextCommand>(cmd.payload);
  EXPECT_EQ(payload.text, "Hello");
  EXPECT_FLOAT_EQ(payload.font_size, 16.0F);
}

TEST(RenderCommandTest, MakeSaveRestore) {
  auto save = RenderCommand::MakeSave();
  auto restore = RenderCommand::MakeRestore();

  EXPECT_EQ(save.type, RenderCommandType::kSave);
  EXPECT_EQ(restore.type, RenderCommandType::kRestore);
}

TEST(RenderCommandTest, MakeTranslate) {
  auto cmd = RenderCommand::MakeTranslate(10.0F, 20.0F);

  EXPECT_EQ(cmd.type, RenderCommandType::kTranslate);
  const auto& payload = std::get<TranslateCommand>(cmd.payload);
  EXPECT_FLOAT_EQ(payload.dx, 10.0F);
  EXPECT_FLOAT_EQ(payload.dy, 20.0F);
}

TEST(RenderCommandTest, MakeClipRect) {
  Rect rect{0.0F, 0.0F, 50.0F, 50.0F};
  auto cmd = RenderCommand::MakeClipRect(rect);

  EXPECT_EQ(cmd.type, RenderCommandType::kClipRect);
  const auto& payload = std::get<ClipRectCommand>(cmd.payload);
  EXPECT_FLOAT_EQ(payload.rect.width, 50.0F);
}

TEST(RenderCommandTest, MakeBeginEndFrame) {
  auto begin = RenderCommand::MakeBeginFrame();
  auto end = RenderCommand::MakeEndFrame();

  EXPECT_EQ(begin.type, RenderCommandType::kBeginFrame);
  EXPECT_EQ(end.type, RenderCommandType::kEndFrame);
}

TEST(RenderContextTest, RecordsCommands) {
  RenderContext ctx;
  ctx.DrawRect({0, 0, 100, 100}, {255, 0, 0, 255});
  ctx.DrawText("Hi", {10, 10}, {0, 0, 0, 255}, 14.0F);
  ctx.Save();
  ctx.Translate(5, 5);
  ctx.Restore();

  EXPECT_EQ(ctx.GetCommandCount(), 5u);
  EXPECT_EQ(ctx.GetCommands()[0].type, RenderCommandType::kDrawRect);
  EXPECT_EQ(ctx.GetCommands()[1].type, RenderCommandType::kDrawText);
  EXPECT_EQ(ctx.GetCommands()[2].type, RenderCommandType::kSave);
  EXPECT_EQ(ctx.GetCommands()[3].type, RenderCommandType::kTranslate);
  EXPECT_EQ(ctx.GetCommands()[4].type, RenderCommandType::kRestore);
}

TEST(RenderContextTest, Clear) {
  RenderContext ctx;
  ctx.DrawRect({0, 0, 10, 10}, {0, 0, 0, 255});
  EXPECT_EQ(ctx.GetCommandCount(), 1u);

  ctx.Clear();
  EXPECT_EQ(ctx.GetCommandCount(), 0u);
  EXPECT_TRUE(ctx.GetCommands().empty());
}

}  // namespace
}  // namespace neoflux
