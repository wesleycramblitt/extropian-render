#pragma once

#include <string>
#include <cstdint>

namespace exd::render {

struct EquirectSkyComponent {
    std::string path;       // path to .hdr equirectangular file
    uint32_t gl_texture = 0; // loaded 2D HDR texture handle
};

} // namespace exd::render
