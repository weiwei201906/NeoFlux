// =============================================================================
// NeoFlux - views/home_view.h
//
// Home page: a landing view with navigation to the other routes. Views are
// ordinary Widgets; keep them in src/views/ (not a top-level widget/ folder).
// =============================================================================

#ifndef NEOFLUX_QUICKSTART_VIEWS_HOME_VIEW_H_
#define NEOFLUX_QUICKSTART_VIEWS_HOME_VIEW_H_

#include <neoflux/widget/widget.h>

namespace neoflux {

// Landing page widget with links to the counter and about pages.
class HomeView final : public StatelessWidget {
 public:
  explicit HomeView(BuildContext& context);

  // Rebuilds the widget subtree for this view.
  std::shared_ptr<Widget> Build(BuildContext& context) override;

 private:
  // Pushes the given route onto the navigation stack.
  void NavigateTo(std::string_view route);
};

}  // namespace neoflux

#endif  // NEOFLUX_QUICKSTART_VIEWS_HOME_VIEW_H_
