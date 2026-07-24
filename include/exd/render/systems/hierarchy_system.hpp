#pragma once

#include <exd/ecs/registry.hpp>

namespace exd::render {

/// Maintains Parent/Children/SiblingLink components.
/// Rebuilds linked lists each frame from Parent references.
/// Domain-agnostic — any entity with render::Parent participates.
///
/// Called once per frame, typically before systems that need hierarchy
/// traversal (canvas compile, physics joints, camera following, etc.).
class HierarchySystem {
public:
    void update(exd::ecs::Registry& registry, double dt);

private:
    void computeDepth(exd::ecs::Registry& registry);
};

} // namespace exd::render
