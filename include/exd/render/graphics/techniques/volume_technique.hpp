#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/draw_data.hpp>
#include <glad/gl.h>
#include <cstdint>

namespace exd::render {

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

} // namespace exd::render
