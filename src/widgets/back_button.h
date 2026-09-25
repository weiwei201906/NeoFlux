// =============================================================================
// NeoFlux - widgets/back_button.h
//
// Reusable navigation widget: a button that pops the current route.
// Shared widgets live in src/widgets/ (page-level components stay in
// src/views/). This demonstrates how to build your own reusable Widgets
// on top of the framework's Button/Container primitives.
// =============================================================================

#ifndef NEOFLUX_QUICKSTART_WIDGETS_BACK_BUTTON_H_
#define NEOFLUX_QUICKSTART_WIDGETS_BACK_BUTTON_H_

#include <neoflux/widget/widget.h>

#include <memory>

namespace neoflux {

// Button that pops the top route from the navigation stack. Use it on any
// page that was pushed (e.g. from the home view) to return to the caller.
class BackButton final : public StatelessWidget {
 public:
  explicit BackButton(BuildContext& context);

  // Builds the button widget.
  std::shared_ptr<Widget> Build(BuildContext& context) override;

 private:
  // Pops the current route. No-op when the stack has a single entry.
  void GoBack();
};

}  // namespace neoflux

#endif  // NEOFLUX_QUICKSTART_WIDGETS_BACK_BUTTON_H_
