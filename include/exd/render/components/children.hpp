#pragma once

#include <cstdint>

namespace exd::render {

/// Linked-list of child entities. Maintained by HierarchySystem.
/// Placed on parent entities; firstChild points to the head of the list.
struct Children {
    uint32_t firstChild = 0;   // Entity::id of first child (0 = no children)
    uint32_t childCount = 0;
};

/// Per-child link in the sibling chain. Maintained by HierarchySystem.
/// Placed on child entities alongside Parent.
struct SiblingLink {
    uint32_t nextSibling = 0;
    uint32_t prevSibling = 0;
};

} // namespace exd::render
