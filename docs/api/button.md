# Button

Clickable widget with a label and press callback.

## Header

```cpp
#include <neoflux/widget/button.h>
```

## Construction

```cpp
auto button = std::make_shared<Button>("Click Me");
```

## Methods

### `SetOnPressed()`

```cpp
Button& SetOnPressed(std::function<void()> callback);
```

Sets the callback invoked when the button is pressed and released within its
bounds.

### `SetLabel()`

```cpp
Button& SetLabel(std::string_view label);
```

Sets the button text.

### `SetFontSize()`

```cpp
Button& SetFontSize(float size);
```

Sets the label font size.

### `SetTextColor()`

```cpp
Button& SetTextColor(const Color& color);
```

Sets the label text color.

### `SetBackgroundColor()`

```cpp
Button& SetBackgroundColor(const Color& color);
```

Sets the button background color.

### `SetBorderRadius()`

```cpp
Button& SetBorderRadius(float radius);
```

Sets the corner radius.

### `SetFont()`

```cpp
Button& SetFont(std::string_view font_name);
```

Sets the label font by name.

## Example

```cpp
auto btn = std::make_shared<Button>("Increment");
btn->SetOnPressed([&counter]() {
    counter++;
    widget->MarkNeedsBuild();
  })
  .SetFontSize(16.0F)
  .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
  .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
  .SetBorderRadius(6.0F);
```

## Behavior

- `OnPointerDown`: Sets `is_pressed_ = true` if the press is within bounds.
- `OnPointerUp`: Invokes `on_pressed_` if the release is within bounds and the
  button was pressed.
- The pressed widget is tracked via `std::weak_ptr` to avoid dangling
  references after rebuilds.
