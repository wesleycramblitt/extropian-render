#pragma once

#include <exd/math/quat.hpp>
#include <cstdint>

namespace exd::render {

/// Per-entity rendering material.
/// Drives shader uniforms (base color, opacity) and GPU state (blend, depth, cull).
/// Emplaced by canvas or any system; consumed by render techniques.
struct Material {
    math::Quat baseColor{0.8f, 0.8f, 0.8f, 1.0f};  // RGBA
    float      opacity   = 1.0f;
    uint8_t    blendMode = 1;   // 0=Opaque, 1=Alpha, 2=Additive, 3=Multiply
    uint8_t    depthMode = 3;   // 0=None, 1=Read, 2=Write, 3=ReadWrite
    uint8_t    cullMode  = 1;   // 0=None, 1=Back, 2=Front
};

} // namespace exd::render
