#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/draw_data.hpp>
#include <glad/gl.h>
#include <cstdint>

namespace exd::render {

class LambertianTechnique {
public:
    explicit LambertianTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind(const math::Mat4& view, const math::Mat4& proj);
    void draw(uint32_t mesh_handle, const math::Mat4& model);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLint u_view_ = -1, u_proj_ = -1, u_model_ = -1, u_light_dir_ = -1;
};

} // namespace exd::render
