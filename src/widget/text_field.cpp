// =============================================================================
// NeoFlux - text_field.cpp
//
// Implementation of the TextField widget.
// =============================================================================

#include "neoflux/widget/text_field.h"

#include <algorithm>
#include <utility>

#include <glog/logging.h>

#include "neoflux/render/render_context.h"

namespace neoflux {

TextField::TextField() {
  SetFocusable(true);
  EnableMeasureFunction();
}

TextField::~TextField() = default;

std::string_view TextField::GetWidgetName() const noexcept {
  return "TextField";
}

Size TextField::OnMeasure(float /*width*/, int /*width_mode*/,
                          float /*height*/, int /*height_mode*/) {
  // Default intrinsic size: width 200, height based on font size + padding.
  return Size{.width = 200.0F, .height = font_size_ + 16.0F};
}

void TextField::Paint(RenderContext& context) {
  const Rect& b = GetBounds();
  if (b.width <= 0.0F || b.height <= 0.0F) {
    return;
  }

  // Background.
  context.DrawRoundedRect(b, background_color_, corner_radius_);

  // Border (draw as a slightly larger rounded rect behind, then fill interior).
  const Color border = HasFocus() ? focus_border_color_ : border_color_;
  if (border_width_ > 0.0F) {
    const Rect border_rect{.x = b.x,
                           .y = b.y,
                           .width = b.width,
                           .height = b.height,};
    context.DrawRoundedRect(border_rect, border, corner_radius_);
    // Inner fill to create border effect.
    const Rect inner{.x = b.x + border_width_,
                     .y = b.y + border_width_,
                     .width = b.width - (2.0F * border_width_),
                     .height = b.height - (2.0F * border_width_),};
    context.DrawRoundedRect(inner, background_color_,
                            corner_radius_ - border_width_);
  }

  // Text or placeholder.
  const float text_x = b.x + 8.0F;
  const float baseline_y = b.y + (b.height * 0.5F) + (font_size_ * 0.35F);
  if (text_.empty() && !placeholder_.empty()) {
    context.DrawText(placeholder_, Point{.x = text_x, .y = baseline_y},
                     placeholder_color_, font_size_);
  } else if (!text_.empty()) {
    context.DrawText(text_, Point{.x = text_x, .y = baseline_y},
                     text_color_, font_size_);
  }

  // Cursor (blink). Only show when focused.
  if (HasFocus()) {
    ++blink_counter_;
    // Cursor visible for ~1 second, invisible for ~1 second at 60fps.
    // 64 frames ~= 1.07s. Use bitwise AND for fast modulo at power-of-two.
    if ((blink_counter_ & 63U) < 32U) {
      // Approximate cursor x position: use font_size * 0.6 per character as
      // a rough estimate. A proper implementation would measure glyph
      // advances, but this is sufficient for the cursor indicator.
      float cursor_x = text_x;
      // Count characters before cursor_pos_ (UTF-8 aware).
      std::size_t char_count = 0;
      for (std::size_t i = 0; i < cursor_pos_ && i < text_.size();) {
        const auto c = static_cast<unsigned char>(text_[i]);
        int seq = 1;
        if (c < 0x80) {
          seq = 1;
        } else if ((c & 0xE0) == 0xC0) {
          seq = 2;
        } else if ((c & 0xF0) == 0xE0) {
          seq = 3;
        } else if ((c & 0xF8) == 0xF0) {
          seq = 4;
        }
        i += static_cast<std::size_t>(seq);
        ++char_count;
      }
      cursor_x += (static_cast<float>(char_count) * font_size_ * 0.6F);
      const Rect cursor{.x = cursor_x,
                        .y = b.y + 4.0F,
                        .width = 2.0F,
                        .height = b.height - 8.0F,};
      context.DrawRect(cursor, cursor_color_);
    }
  } else {
    blink_counter_ = 0;
  }
}

bool TextField::OnPointerDown(const Point& local_pos) {
  // Set focus is handled by Application dispatch. Position cursor based on
  // click x coordinate (approximate character width).
  const float char_width = font_size_ * 0.6F;
  const float text_x = 8.0F;
  const float rel_x = local_pos.x - text_x;
  if (rel_x <= 0.0F) {
    cursor_pos_ = 0;
  } else {
    const auto target_char =
        static_cast<std::size_t>((rel_x / char_width) + 0.5F);
    // Find byte offset for the target character.
    std::size_t char_count = 0;
    std::size_t byte_offset = 0;
    while (byte_offset < text_.size() && char_count < target_char) {
      const auto c = static_cast<unsigned char>(text_[byte_offset]);
      int seq = 1;
      if (c < 0x80) {
        seq = 1;
      } else if ((c & 0xE0) == 0xC0) {
        seq = 2;
      } else if ((c & 0xF0) == 0xE0) {
        seq = 3;
      } else if ((c & 0xF8) == 0xF0) {
        seq = 4;
      }
      byte_offset += static_cast<std::size_t>(seq);
      ++char_count;
    }
    cursor_pos_ = byte_offset;
  }
  blink_counter_ = 0;  // Reset blink so cursor is immediately visible.
  return true;
}

bool TextField::OnKeyEvent(const KeyEvent& event) {
  if (!event.pressed) {
    return false;  // Only handle key press (not release).
  }
  const bool ctrl =
      (event.modifiers & static_cast<std::uint8_t>(KeyModifiers::kControl)) !=
      0;
  // Ctrl+A: select all (move cursor to end).
  if (event.key == KeyCode::kA && ctrl) {
    cursor_pos_ = text_.size();
    return true;
  }
  switch (event.key) {
    case KeyCode::kBackspace:
      DeleteBackward();
      return true;
    case KeyCode::kDelete:
      DeleteForward();
      return true;
    case KeyCode::kLeft:
      MoveCursorLeft();
      return true;
    case KeyCode::kRight:
      MoveCursorRight();
      return true;
    case KeyCode::kHome:
      MoveCursorToStart();
      return true;
    case KeyCode::kEnd:
      MoveCursorToEnd();
      return true;
    case KeyCode::kEnter:
      if (on_submit_ != nullptr) {
        on_submit_(text_);
      }
      return true;
    default:
      return false;
  }
}

bool TextField::OnCharEvent(const CharEvent& event) {
  // Ignore control characters (handled by OnKeyEvent).
  if (event.codepoint < 32U) {
    return false;
  }
  VLOG(1) << "TextField::OnCharEvent codepoint=" << event.codepoint;
  InsertCodepoint(event.codepoint);
  return true;
}

void TextField::OnFocus() { blink_counter_ = 0; }

void TextField::OnBlur() { blink_counter_ = 0; }

void TextField::SetText(std::string_view text) {
  text_ = std::string(text);
  cursor_pos_ = text_.size();
  if (on_change_ != nullptr) {
    on_change_(text_);
  }
}

std::string_view TextField::GetText() const noexcept { return text_; }

void TextField::SetPlaceholder(std::string_view placeholder) {
  placeholder_ = std::string(placeholder);
}

void TextField::SetFontSize(float size) noexcept { font_size_ = size; }

void TextField::SetOnSubmit(SubmitCallback callback) {
  on_submit_ = std::move(callback);
}

void TextField::SetOnChange(ChangeCallback callback) {
  on_change_ = std::move(callback);
}

void TextField::SetTextColor(const Color& color) noexcept {
  text_color_ = color;
}

void TextField::SetPlaceholderColor(const Color& color) noexcept {
  placeholder_color_ = color;
}

void TextField::SetBackgroundColor(const Color& color) noexcept {
  background_color_ = color;
}

void TextField::SetBorderColor(const Color& color) noexcept {
  border_color_ = color;
}

void TextField::SetCursorColor(const Color& color) noexcept {
  cursor_color_ = color;
}

void TextField::SetBorderWidth(float width) noexcept {
  border_width_ = width;
}

void TextField::SetCornerRadius(float radius) noexcept {
  corner_radius_ = radius;
}

void TextField::InsertCodepoint(std::uint32_t codepoint) {
  std::string utf8;
  CodepointToUtf8(codepoint, utf8);
  text_.insert(cursor_pos_, utf8);
  cursor_pos_ += utf8.size();
  if (on_change_ != nullptr) {
    on_change_(text_);
  }
}

void TextField::DeleteBackward() {
  if (cursor_pos_ == 0) {
    return;
  }
  const std::size_t prev = PrevCharOffset(cursor_pos_);
  text_.erase(prev, cursor_pos_ - prev);
  cursor_pos_ = prev;
  if (on_change_ != nullptr) {
    on_change_(text_);
  }
}

void TextField::DeleteForward() {
  if (cursor_pos_ >= text_.size()) {
    return;
  }
  const std::size_t next = NextCharOffset(cursor_pos_);
  text_.erase(cursor_pos_, next - cursor_pos_);
  if (on_change_ != nullptr) {
    on_change_(text_);
  }
}

void TextField::MoveCursorLeft() {
  if (cursor_pos_ > 0) {
    cursor_pos_ = PrevCharOffset(cursor_pos_);
  }
}

void TextField::MoveCursorRight() {
  if (cursor_pos_ < text_.size()) {
    cursor_pos_ = NextCharOffset(cursor_pos_);
  }
}

void TextField::MoveCursorToStart() { cursor_pos_ = 0; }

void TextField::MoveCursorToEnd() { cursor_pos_ = text_.size(); }

std::size_t TextField::PrevCharOffset(std::size_t from) const noexcept {
  if (from == 0) {
    return 0;
  }
  std::size_t pos = from - 1;
  // Walk backward over UTF-8 continuation bytes (10xxxxxx).
  while (pos > 0 &&
         (static_cast<unsigned char>(text_[pos]) & 0xC0U) == 0x80U) {
    --pos;
  }
  return pos;
}

std::size_t TextField::NextCharOffset(std::size_t from) const noexcept {
  if (from >= text_.size()) {
    return text_.size();
  }
  const auto c = static_cast<unsigned char>(text_[from]);
  int seq = 1;
  if (c < 0x80) {
    seq = 1;
  } else if ((c & 0xE0) == 0xC0) {
    seq = 2;
  } else if ((c & 0xF0) == 0xE0) {
    seq = 3;
  } else if ((c & 0xF8) == 0xF0) {
    seq = 4;
  }
  return std::min(from + static_cast<std::size_t>(seq), text_.size());
}

void TextField::CodepointToUtf8(std::uint32_t codepoint, std::string& out) {
  if (codepoint < 0x80U) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint < 0x800U) {
    out.push_back(static_cast<char>(0xC0U | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
  } else if (codepoint < 0x10000U) {
    out.push_back(static_cast<char>(0xE0U | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80U | ((codepoint >> 12) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
  }
}

}  // namespace neoflux
