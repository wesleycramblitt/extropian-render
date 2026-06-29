#pragma once

#include <ext/render/mesh_manager.hpp>
#include <ext/render/texture_manager.hpp>
#include <ext/render/shader_manager.hpp>

namespace ext::render {

/// Aggregates all GPU resource managers. A single GraphicsContext is shared
/// by all render techniques and systems within an application.
struct GraphicsContext {
    MeshManager    mesh_manager;
    ShaderManager  shader_manager;
    TextureManager texture_manager;
};

} // namespace ext::render
