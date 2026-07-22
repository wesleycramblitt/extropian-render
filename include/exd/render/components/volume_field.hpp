#pragma once

#include <cstdint>

namespace exd::render {

struct VolumeFieldComponent {
    uint32_t texture_handle = 0;
    bool interop_ready = false;
};

} // namespace exd::render
