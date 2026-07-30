// HierarchySystem — maintains Parent/Children/SiblingLink components.

#include <exd/render/systems/hierarchy_system.hpp>
#include <exd/render/components/parent.hpp>
#include <exd/render/components/children.hpp>
#include <exd/ecs/view.hpp>

namespace exd::render {

void HierarchySystem::update(exd::ecs::Registry& registry, double /*dt*/) {
    // Phase 1: clear all Children components
    auto childView = registry.view<Children>();
    for (auto e : childView) {
        auto& ch = registry.get<Children>(e);
        ch.firstChild = 0;
        ch.childCount = 0;
    }

    // Phase 2: rebuild linked lists from Parent references
    auto parentView = registry.view<Parent>();
    for (auto e : parentView) {
        auto& parent = registry.get<Parent>(e);
        if (parent.entityId == 0xFFFFFFFF) continue;

        exd::ecs::Entity parentEntity{parent.entityId, 0};
        if (!registry.valid(parentEntity)) continue;

        auto& children = registry.has<Children>(parentEntity)
            ? registry.get<Children>(parentEntity)
            : registry.emplace<Children>(parentEntity);

        auto& link = registry.has<SiblingLink>(e)
            ? registry.get<SiblingLink>(e)
            : registry.emplace<SiblingLink>(e);

        link.nextSibling = children.firstChild;
        link.prevSibling = 0;

        if (children.firstChild != 0) {
            exd::ecs::Entity oldFirst{children.firstChild, 0};
            if (registry.valid(oldFirst) && registry.has<SiblingLink>(oldFirst)) {
                registry.get<SiblingLink>(oldFirst).prevSibling = e.id;
            }
        }

        children.firstChild = e.id;
        children.childCount++;
    }

    // Phase 3: compute depth
    computeDepth(registry);
}

void HierarchySystem::computeDepth(exd::ecs::Registry& registry) {
    // Find roots (entities with no Parent), perform depth-first traversal.
    // TODO: store depth on entities when a Depth component is added.
    (void)registry;
}

} // namespace exd::render
