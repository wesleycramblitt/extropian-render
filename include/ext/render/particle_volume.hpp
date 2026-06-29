#pragma once

#include <ext/render/graphics_context.hpp>
#include <ext/render/draw_data.hpp>
#include <glad/gl.h>
#include <vector>

namespace ext::render {

class ParticleRenderTechnique {
public:
    explicit ParticleRenderTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind();
    void draw(const ParticleDrawData& data);
    void unbind();

private:
    struct GLState {
        GLuint vao = 0;
        GLuint vbo = 0;
        int capacity = 0;
    };

    void init_gl(GLState& s, int capacity);
    void upload(GLState& s, const float* positions, const float* colors, int count);

    GraphicsContext& ctx_;
    uint32_t program_ = 0;
    GLState state_;
};

class VolumeRenderTechnique {
public:
    explicit VolumeRenderTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    void bind();
    void draw(const VolumeDrawData& data);
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
};

} // namespace ext::render
