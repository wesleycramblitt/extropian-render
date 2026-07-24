#include <exd/render/interaction/selection.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/hovered.hpp>
#include <exd/render/components/disabled.hpp>
#include <vector>

namespace exd::render {

void SelectionSystem::handle_click(ecs::Registry& registry,
                                    std::optional<ecs::Entity> hit,
                                    bool shift_held) {
    // Collect hovered entities to clear (can't modify while iterating view)
    std::vector<ecs::Entity> to_clear_hover;
    for (auto e : registry.view<Hovered>()) to_clear_hover.push_back(e);
    for (auto e : to_clear_hover) registry.remove<Hovered>(e);

    if (hit && registry.valid(*hit)) {
        registry.emplace<Hovered>(*hit);

        if (shift_held) {
            if (registry.has<Selected>(*hit)) {
                registry.remove<Selected>(*hit);
            } else {
                registry.emplace<Selected>(*hit);
            }
        } else {
            // Collect then remove — can't modify while iterating view
            std::vector<ecs::Entity> to_deselect;
            for (auto e : registry.view<Selected>()) to_deselect.push_back(e);
            for (auto e : to_deselect) registry.remove<Selected>(e);
            registry.emplace<Selected>(*hit);
        }
    } else {
        std::vector<ecs::Entity> to_deselect;
        for (auto e : registry.view<Selected>()) to_deselect.push_back(e);
        for (auto e : to_deselect) registry.remove<Selected>(e);
    }
}

void SelectionSystem::clear_all(ecs::Registry& registry) {
    std::vector<ecs::Entity> sel, hov;
    for (auto e : registry.view<Selected>()) sel.push_back(e);
    for (auto e : registry.view<Hovered>())  hov.push_back(e);
    for (auto e : sel) registry.remove<Selected>(e);
    for (auto e : hov) registry.remove<Hovered>(e);
}

int SelectionSystem::selection_count(ecs::Registry& registry) const {
    int count = 0;
    for (auto e : registry.view<Selected>()) {
        (void)e;
        ++count;
    }
    return count;
}

} // namespace exd::render
