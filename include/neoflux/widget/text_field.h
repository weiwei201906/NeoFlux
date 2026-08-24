// =============================================================================
// NeoFlux - text_field.h
//
// Single-line text input widget. Supports text entry, cursor navigation,
// selection, and placeholder text. Keyboard focus is required to receive
// input; click the field to focus it.
//
// All method implementations are in src/widget/text_field.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_TEXT_FIELD_H_
#define NEOFLUX_WIDGET_TEXT_FIELD_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "neoflux/widget/widget.h"

namespace neoflux {

// Single-line editable text field.
//
// Usage:
//   auto field = std::make_shared<TextField>();
//   field->SetPlaceholder("Enter text...");
//   field->SetOnSubmit([](std::string_view text) { ... });
//   container->AddChild(field);
//
// The field is focusable by default. Clicking it sets keyboard focus and
// positions the cursor. Keyboard input is delivered via OnCharEvent and
// OnKeyEvent from the Application dispatch layer.
class TextField : public Widget {
 public:
  // Callback invoked when the user presses Enter.
  using SubmitCallback = std::function<void(std::string_view text)>;

  // Callback invoked whenever the text content changes.
  using ChangeCallback = std::function<void(std::string_view text)>;

  TextField();
  ~TextField() override;

  // Returns the human-readable widget name for debugging.
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Paints the text field: background, border, text, cursor, placeholder.
  // Reports intrinsic text field size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  void Paint(RenderContext& context) override;

  // Handles a pointer press: sets focus and moves the cursor to the click.
  bool OnPointerDown(const Point& local_pos) override;

  // Handles keyboard key events (cursor movement, deletion, submission).
  bool OnKeyEvent(const KeyEvent& event) override;

  // Handles Unicode character input (inserts at cursor position).
  bool OnCharEvent(const CharEvent& event) override;

  // Called when the field gains keyboard focus.
  void OnFocus() override;

  // Called when the field loses keyboard focus.
  void OnBlur() override;

  // --- Configuration ---

  // Sets the current text content.
  void SetText(std::string_view text);

  // Returns the current text content.
  [[nodiscard]] std::string_view GetText() const noexcept;

  // Sets the placeholder text shown when the field is empty.
  void SetPlaceholder(std::string_view placeholder);

  // Sets the font size in pixels.
  void SetFontSize(float size) noexcept;

  // Sets the callback invoked on Enter press.
  void SetOnSubmit(SubmitCallback callback);

  // Sets the callback invoked on every text change.
  void SetOnChange(ChangeCallback callback);

  // Sets the text color.
  void SetTextColor(const Color& color) noexcept;

  // Sets the placeholder text color.
  void SetPlaceholderColor(const Color& color) noexcept;

  // Sets the background color.
  void SetBackgroundColor(const Color& color) noexcept;

  // Sets the border color.
  void SetBorderColor(const Color& color) noexcept;

  // Sets the cursor color.
  void SetCursorColor(const Color& color) noexcept;

  // Sets the border width in pixels.
  void SetBorderWidth(float width) noexcept;

  // Sets the corner radius in pixels (0 = sharp corners).
  void SetCornerRadius(float radius) noexcept;

 private:
  // Inserts a Unicode code point at the current cursor position.
  void InsertCodepoint(std::uint32_t codepoint);

  // Deletes the character before the cursor (Backspace).
  void DeleteBackward();

  // Deletes the character after the cursor (Delete).
  void DeleteForward();

  // Moves the cursor left by one character.
  void MoveCursorLeft();

  // Moves the cursor right by one character.
  void MoveCursorRight();

  // Moves the cursor to the start of the text.
  void MoveCursorToStart();

  // Moves the cursor to the end of the text.
  void MoveCursorToEnd();

  // Returns the byte offset of the character before the cursor, or 0 if at
  // the start. Handles multi-byte UTF-8 sequences.
  [[nodiscard]] std::size_t PrevCharOffset(std::size_t from) const noexcept;

  // Returns the byte offset of the character after the cursor, or text.size()
  // if at the end. Handles multi-byte UTF-8 sequences.
  [[nodiscard]] std::size_t NextCharOffset(std::size_t from) const noexcept;

  // Converts a Unicode code point to UTF-8 and appends it to the string.
  static void CodepointToUtf8(std::uint32_t codepoint, std::string& out);

  // Counts the number of UTF-8 characters in the first 'byte_limit' bytes
  // of 'text'. Used to approximate cursor x position from byte offset.
  static std::size_t Utf8CharCount(std::string_view text,
                                   std::size_t byte_limit) noexcept;

  // Returns true if there is an active text selection (anchor != cursor).
  [[nodiscard]] bool HasSelection() const noexcept;

  // Returns the selected text (empty if no selection).
  [[nodiscard]] std::string_view GetSelectedText() const noexcept;

  // Returns the selection range [start, end) in byte offsets, normalized
  // so start <= end.
  void GetSelectionRange(std::size_t& start, std::size_t& end) const noexcept;

  // Deletes the selected text and clears the selection. Returns true if
  // text was deleted.
  bool DeleteSelection();

  // Selects all text (anchor=0, cursor=end).
  void SelectAll() noexcept;

  // Clears the selection without deleting text (anchor = cursor).
  void ClearSelection() noexcept;

  std::string text_{};
  std::string placeholder_{};
  std::size_t cursor_pos_ = 0;  // Byte offset into text_.
  std::size_t selection_anchor_ = 0;  // Selection anchor byte offset. When
                                      // equal to cursor_pos_, no selection.
  float font_size_ = 16.0F;
  float border_width_ = 1.0F;
  float corner_radius_ = 4.0F;

  Color text_color_{.r = 30, .g = 30, .b = 30, .a = 255};
  Color placeholder_color_{.r = 150, .g = 150, .b = 150, .a = 255};
  Color background_color_{.r = 255, .g = 255, .b = 255, .a = 255};
  Color border_color_{.r = 200, .g = 200, .b = 200, .a = 255};
  Color cursor_color_{.r = 0, .g = 0, .b = 0, .a = 255};
  Color focus_border_color_{.r = 66, .g = 133, .b = 244, .a = 255};

  SubmitCallback on_submit_{};
  ChangeCallback on_change_{};

  // Frame counter for cursor blink. Incremented each Paint; cursor visible
  // when (blink_counter_ & 64) == 0 (roughly 1 second at 60fps).
  std::uint64_t blink_counter_ = 0;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_TEXT_FIELD_H_
