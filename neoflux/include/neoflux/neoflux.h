// =============================================================================
// NeoFlux - neoflux.h
//
// Umbrella header that includes the entire public API of the NeoFlux
// framework. User code can include this single header to get access to
// all widgets, the application, and core utilities.
// =============================================================================

#ifndef NEOFLUX_NEOFLUX_H_
#define NEOFLUX_NEOFLUX_H_

// Core utilities.
#include "neoflux/core/macros.h"
#include "neoflux/core/noncopyable.h"
#include "neoflux/core/ring_queue.h"
#include "neoflux/core/task.h"
#include "neoflux/core/types.h"

// Widget system.
#include "neoflux/widgets/button.h"
#include "neoflux/widgets/container.h"
#include "neoflux/widgets/expanded.h"
#include "neoflux/widgets/draggable.h"
#include "neoflux/widgets/route_registry.h"
#include "neoflux/widgets/scroll_view.h"
#include "neoflux/widgets/sized_box.h"
#include "neoflux/widgets/text.h"
#include "neoflux/widgets/widget.h"

// Application layer.
#include "neoflux/apps/application.h"
#include "neoflux/apps/event_loop.h"

// Render layer.
#include "neoflux/renderers/render_command.h"
#include "neoflux/renderers/render_context.h"
#include "neoflux/renderers/render_layer.h"

#endif  // NEOFLUX_NEOFLUX_H_
