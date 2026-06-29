#pragma once

#include <exd/render/uniform_value.hpp>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace exd::render {

struct Renderable {
    uint32_t mesh_handle = 0;
    uint32_t texture_handle = 0;
    std::unordered_map<std::string, UniformValue> uniforms;
};

struct ParticleDrawData {
    const float* positions = nullptr;
    const float* colors = nullptr;  // may be nullptr for solid-color fallback
    int count = 0;
    std::unordered_map<std::string, UniformValue> uniforms;
};

struct VolumeDrawData {
    uint32_t texture_handle = 0;
    uint32_t proxy_mesh = 0;
    int nx = 0, ny = 0, nz = 0;
    std::unordered_map<std::string, UniformValue> uniforms;
};

} // namespace exd::render
