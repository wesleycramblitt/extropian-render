#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/math/mat4.hpp>
#include <exd/render/graphics/gl_loader.hpp>
#include <cstdint>

namespace exd::render {

class ReflectiveTechnique {
public:
    explicit ReflectiveTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind(const math::Mat4& view, const math::Mat4& proj,
              const math::Vec3f& cam_pos, uint32_t cubemap_handle);
    void draw(uint32_t mesh_handle, const math::Mat4& model);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLint u_view_ = -1, u_proj_ = -1, u_model_ = -1, u_cam_pos_ = -1, u_skybox_ = -1;
};

} // namespace exd::render
