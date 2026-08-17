# ScrollView

A scrollable viewport that clips and pans its content. Content larger than the
viewport can be scrolled via mouse wheel (desktop) or touch drag (mobile).

## Header

```cpp
#include <neoflux/widget/scroll_view.h>
```

## Construction

```cpp
auto scroll = std::make_shared<ScrollView>();
```

## Methods

### `SetContent()`

```cpp
void SetContent(std::shared_ptr<Widget> content);
```

Sets the scrollable content. Replaces any existing content. The content is
laid out at its natural size and clipped to the viewport.

### `ScrollTo()`

```cpp
void ScrollTo(float x, float y) noexcept;
```

Sets the scroll offset in pixels. Clamped to the valid range.

### `GetScrollOffset()`

```cpp
[[nodiscard]] Point GetScrollOffset() const noexcept;
```

Returns the current scroll offset.

### `GetContentSize()`

```cpp
[[nodiscard]] Size GetContentSize() const noexcept;
```

Returns the content size after layout.

## Layout Behavior

- `ScrollView` sets `flex_grow = 1` and `flex_shrink = 1` to fill available
  space in its parent.
- Content is set to `flex_shrink = 0` and `align_self = flex-start` so it keeps
  its natural size.
- `overflow = hidden` clips content to the viewport.

## Example

```cpp
auto scroll = std::make_shared<ScrollView>();

auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);
for (int i = 0; i < 20; ++i) {
  auto item = std::make_shared<Text>("Item " + std::to_string(i));
  content->AddChild(item);
}

scroll->SetContent(content);
parent->AddChild(scroll);
```

## Painting

`ScrollView::Paint()`:
1. Saves the current transform/clip state.
2. Applies a clip rect to the viewport bounds.
3. Translates by the negative scroll offset.
4. Paints children.
5. Restores the previous state.
