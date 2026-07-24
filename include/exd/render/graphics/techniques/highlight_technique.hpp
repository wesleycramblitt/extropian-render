#pragma once

#include <exd/render/graphics/graphics_context.hpp>
#include <exd/math/mat4.hpp>
#include <glad/gl.h>
#include <cstdint>

namespace exd::render {

/// Renders a highlight overlay on selected entities.
///
/// Two modes available per draw call:
///   Outline — render mesh scaled up ~3% with solid color, no depth write
///   Wireframe — render mesh edges with glPolygonMode(GL_LINE)
///
/// The highlight color defaults to orange (#FF8800) but can be overridden.
class HighlightTechnique {
public:
    explicit HighlightTechnique(GraphicsContext& ctx) : ctx_(ctx) {}

    /// Set up highlight shader and shared uniforms.
    void bind(const math::Mat4& view, const math::Mat4& proj,
              const math::Vec3f& color = {1.0f, 0.53f, 0.0f});

    /// Draw a highlighted mesh.
    /// `model` should already include the outline scale-up if using outline mode.
    void draw(uint32_t mesh_handle, const math::Mat4& model);

    /// Restore render state.
    void unbind();

private:
    GraphicsContext& ctx_;
    uint32_t program_ = 0;
};

} // namespace exd::render
