#pragma once

#include <memory>
#include <string_view>
#include <cstdint>

namespace ext::render {

class RenderGraph;
class Camera;

/// @brief Abstract 3D renderer interface.
///
/// Each backend (OpenGL, Vulkan, WebGL, Null) implements this.
/// Applications submit a RenderGraph each frame.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// ── Lifecycle ─────────────────────────────────────
    virtual void initialize(void* window_handle) = 0;
    virtual void shutdown() = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;

    /// ── Frame ─────────────────────────────────────────
    virtual void begin_frame() = 0;
    virtual void execute(const RenderGraph& graph, const Camera& camera) = 0;
    virtual void end_frame() = 0;

    /// ── Resource creation ─────────────────────────────
    /// All resource types (mesh, texture, shader) are created through
    /// the renderer and returned as opaque handles.

    /// ── Backend info ──────────────────────────────────
    [[nodiscard]] virtual std::string_view backend_name() const = 0;
    [[nodiscard]] virtual std::string_view renderer_info() const = 0;

    /// ── Factory ───────────────────────────────────────
    enum class Backend { OpenGL, Vulkan, WebGL, Null };
    static std::unique_ptr<IRenderer> create(Backend backend);
};

} // namespace ext::render
