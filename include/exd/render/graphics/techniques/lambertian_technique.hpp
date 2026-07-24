#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/draw_data.hpp>
#include <exd/render/components/material.hpp>
#include <exd/math/vec3.hpp>
#include <glad/gl.h>
#include <cstdint>

namespace exd::render {

class LambertianTechnique {
public:
    explicit LambertianTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind(const math::Mat4& view, const math::Mat4& proj,
              const math::Vec3f& cam_pos = {0.0f, 0.0f, 0.0f},
              const math::Vec3f& fog_color = {0.5f, 0.5f, 0.5f},
              float fog_density = 0.0f,
              const math::Vec3f& ambient = {0.1f, 0.1f, 0.1f},
              const math::Vec3f& sun_dir = {0.5f, 1.0f, 0.3f},
              const math::Vec3f& sun_color = {1.0f, 1.0f, 1.0f});
    void draw(uint32_t mesh_handle, const math::Mat4& model);
    void setBaseColor(const math::Quat& color);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLint u_view_ = -1, u_proj_ = -1, u_model_ = -1, u_light_dir_ = -1, u_baseColor_ = -1;
    GLint u_fog_color_ = -1, u_fog_density_ = -1;
    GLint u_ambient_ = -1, u_sun_color_ = -1, u_cam_pos_ = -1;
};

} // namespace exd::render
