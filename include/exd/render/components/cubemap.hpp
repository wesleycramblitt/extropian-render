#pragma once

#include <string>
#include <cstdint>

namespace exd::render {

struct CubeMapComponent {
    std::string name;
    bool cross_layout = true;
    uint32_t texture_handle = 0;
    uint32_t gl_cubemap = 0;  // OpenGL cubemap texture ID
};

} // namespace exd::render
