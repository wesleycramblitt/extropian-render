#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/draw_data.hpp>
#include <exd/render/graphics/gl_loader.hpp>

namespace exd::render {

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

} // namespace exd::render
