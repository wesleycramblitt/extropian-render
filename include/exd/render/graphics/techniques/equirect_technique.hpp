#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/math/mat4.hpp>
#include <cstdint>

namespace exd::render {

/// Draws an equirectangular 2D texture mapped onto a sphere (sky dome).
class EquirectTechnique {
public:
    explicit EquirectTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind(const math::Mat4& view, const math::Mat4& proj);
    void draw(uint32_t mesh_handle, uint32_t texture_handle);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
};

} // namespace exd::render
