#pragma once

#include <cstdint>

namespace exd::render {

/// Generic parent-child relationship for ECS entities.
/// Any entity with this component is a child of the entity referenced by entityId.
/// entityId == 0 means no parent (root).
///
/// Used by canvas for visual hierarchy, by physics for bone chains,
/// by app for camera following, etc. HierarchySystem composes world transforms
/// from the Parent chain.
struct Parent {
    uint32_t entityId = 0xFFFFFFFF;   // UINT32_MAX = no parent (root).
                                       // 0 is a valid entity id from Registry::create().
};

} // namespace exd::render
