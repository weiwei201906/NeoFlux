// =============================================================================
// NeoFlux - views/counter_view.h
//
// Counter demo: a StatefulWidget with a working increment button. Shows how
// to use State<StatefulWidget> + SetState to rebuild a subtree on user
// interaction. The counter value lives in the state object, which survives
// widget rebuilds.
// =============================================================================

#ifndef NEOFLUX_QUICKSTART_VIEWS_COUNTER_VIEW_H_
#define NEOFLUX_QUICKSTART_VIEWS_COUNTER_VIEW_H_

#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {

// Forward declaration used by the state class below.
class CounterView;

// State object holding the counter value; persists across widget rebuilds.
class CounterViewState final : public State<StatefulWidget> {
 public:
  // Builds the counter page using the current count.
  std::shared_ptr<Widget> Build(BuildContext& context) override;

 private:
  // Increments the counter and schedules a rebuild.
  void Increment();

  int count_ = 0;
};

// Page demonstrating stateful widgets: a button that increments a counter.
class CounterView final : public StatefulWidget {
 public:
  explicit CounterView(BuildContext& /*context*/);

  // Creates the state object attached to this widget.
  [[nodiscard]] std::unique_ptr<State<StatefulWidget>> CreateState() override;
};

}  // namespace neoflux

#endif  // NEOFLUX_QUICKSTART_VIEWS_COUNTER_VIEW_H_
