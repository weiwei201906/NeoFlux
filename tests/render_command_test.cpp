// =============================================================================
// NeoFlux - render_command_test.cpp
//
// Unit tests for render command creation and the render context.
// =============================================================================

#include <neoflux/render/render_command.h>
#include <neoflux/render/render_context.h>

#include <neoflux/core/types.h>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

TEST(RenderCommandTest, MakeDrawRect) {
  Rect rect{.x = 10.0F, .y = 20.0F, .width = 100.0F, .height = 50.0F};
  Color color{.r = 255, .g = 0, .b = 0, .a = 255};
  auto cmd = RenderCommand::MakeDrawRect(rect, color);

  EXPECT_EQ(cmd.type, RenderCommandType::kDrawRect);
  EXPECT_FLOAT_EQ(cmd.rect.x, 10.0F);
  EXPECT_FLOAT_EQ(cmd.rect.width, 100.0F);
  EXPECT_EQ(cmd.color.r, 255);
}

TEST(RenderCommandTest, MakeDrawText) {
  auto cmd = RenderCommand::MakeDrawText(
      "Hello", Point{.x = 5.0F, .y = 10.0F},
      Color{.r = 0, .g = 0, .b = 0, .a = 255}, 16.0F, "");

  EXPECT_EQ(cmd.type, RenderCommandType::kDrawText);
  EXPECT_EQ(cmd.text, "Hello");
  EXPECT_FLOAT_EQ(cmd.font_size, 16.0F);
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
  EXPECT_FLOAT_EQ(cmd.translate_x, 10.0F);
  EXPECT_FLOAT_EQ(cmd.translate_y, 20.0F);
}

TEST(RenderCommandTest, MakeClipRect) {
  Rect rect{.x = 0.0F, .y = 0.0F, .width = 50.0F, .height = 50.0F};
  auto cmd = RenderCommand::MakeClipRect(rect);

  EXPECT_EQ(cmd.type, RenderCommandType::kClipRect);
  EXPECT_FLOAT_EQ(cmd.rect.width, 50.0F);
}

TEST(RenderCommandTest, MakeBeginEndFrame) {
  auto begin = RenderCommand::MakeBeginFrame();
  auto end = RenderCommand::MakeEndFrame();

  EXPECT_EQ(begin.type, RenderCommandType::kBeginFrame);
  EXPECT_EQ(end.type, RenderCommandType::kEndFrame);
}

TEST(RenderContextTest, RecordsCommands) {
  RenderContext ctx;
  ctx.DrawRect(Rect{.x = 0, .y = 0, .width = 100, .height = 100},
               Color{.r = 255, .g = 0, .b = 0, .a = 255});
  ctx.DrawText("Hi", Point{.x = 10, .y = 10},
               Color{.r = 0, .g = 0, .b = 0, .a = 255}, 14.0F);
  ctx.Save();
  ctx.Translate(5, 5);
  ctx.Restore();

  EXPECT_EQ(ctx.GetCommandCount(), 5U);
  EXPECT_EQ(ctx.GetCommands()[0].type, RenderCommandType::kDrawRect);
  EXPECT_EQ(ctx.GetCommands()[1].type, RenderCommandType::kDrawText);
  EXPECT_EQ(ctx.GetCommands()[2].type, RenderCommandType::kSave);
  EXPECT_EQ(ctx.GetCommands()[3].type, RenderCommandType::kTranslate);
  EXPECT_EQ(ctx.GetCommands()[4].type, RenderCommandType::kRestore);
}

TEST(RenderContextTest, Clear) {
  RenderContext ctx;
  ctx.DrawRect(Rect{.x = 0, .y = 0, .width = 10, .height = 10},
               Color{.r = 0, .g = 0, .b = 0, .a = 255});
  EXPECT_EQ(ctx.GetCommandCount(), 1U);

  ctx.Clear();
  EXPECT_EQ(ctx.GetCommandCount(), 0U);
  EXPECT_TRUE(ctx.GetCommands().empty());
}

}  // namespace
}  // namespace neoflux
