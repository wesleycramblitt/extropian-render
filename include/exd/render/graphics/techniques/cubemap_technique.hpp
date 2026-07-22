#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/draw_data.hpp>
#include <cstdint>

namespace exd::render {

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
