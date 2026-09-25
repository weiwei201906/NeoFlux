// =============================================================================
// NeoFlux - views/about_view.h
//
// About page: a simple informational view. Demonstrates a minimal
// StatelessWidget that only renders content (no interaction).
// =============================================================================

#ifndef NEOFLUX_QUICKSTART_VIEWS_ABOUT_VIEW_H_
#define NEOFLUX_QUICKSTART_VIEWS_ABOUT_VIEW_H_

#include <neoflux/widget/widget.h>

#include <memory>

namespace neoflux {

// About page showing framework version and architecture summary.
class AboutView final : public StatelessWidget {
 public:
  explicit AboutView(BuildContext& context);

  // Builds the about page content.
  std::shared_ptr<Widget> Build(BuildContext& context) override;
};

}  // namespace neoflux

#endif  // NEOFLUX_QUICKSTART_VIEWS_ABOUT_VIEW_H_
