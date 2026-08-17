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
#include "neoflux/widget/button.h"
#include "neoflux/widget/container.h"
#include "neoflux/widget/expanded.h"
#include "neoflux/widget/route_registry.h"
#include "neoflux/widget/scroll_view.h"
#include "neoflux/widget/sized_box.h"
#include "neoflux/widget/text.h"
#include "neoflux/widget/widget.h"

// Application layer.
#include "neoflux/app/application.h"
#include "neoflux/app/event_loop.h"

// Render layer.
#include "neoflux/render/render_command.h"
#include "neoflux/render/render_context.h"
#include "neoflux/render/render_layer.h"

#endif  // NEOFLUX_NEOFLUX_H_
