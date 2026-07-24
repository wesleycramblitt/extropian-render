#pragma once

#include <exd/ecs/registry.hpp>
#include <optional>

namespace exd::render {

/// Manages entity selection state via the `Selected` and `Hovered` markers.
///
/// Click-select:
///   - Left-click on entity → toggles `Selected` on that entity
///   - Shift+click → adds/removes from selection
///   - Click empty space → clears all `Selected`
///
/// Multi-select (future):
///   - Rubber-band drag → selects all entities within screen rect
class SelectionSystem {
public:
    SelectionSystem() = default;

    /// Process a click at screen coordinates. Returns the hit entity if any.
    /// Updates Selected/Hovered markers on the registry.
    /// `shift_held` enables additive selection.
    void handle_click(ecs::Registry& registry,
                      std::optional<ecs::Entity> hit,
                      bool shift_held);

    /// Clear all Selected and Hovered markers.
    void clear_all(ecs::Registry& registry);

    /// Returns the number of currently selected entities.
    int selection_count(ecs::Registry& registry) const;
};

} // namespace exd::render
