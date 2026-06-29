#pragma once

#include <exd/render/graphics_context.hpp>
#include <exd/render/draw_data.hpp>
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

class ReflectiveTechnique {
public:
    explicit ReflectiveTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind(const math::Mat4& view, const math::Mat4& proj,
              const math::Vec3& cam_pos, uint32_t cubemap_handle);
    void draw(uint32_t mesh_handle, const math::Mat4& model);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLint u_view_ = -1, u_proj_ = -1, u_model_ = -1, u_cam_pos_ = -1, u_skybox_ = -1;
};

class CubeMapRenderTechnique {
public:
    explicit CubeMapRenderTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind();
    void draw(const Renderable& renderable);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t cubemap_program_ = 0;
};

} // namespace exd::render
