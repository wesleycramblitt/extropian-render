#include <exd/render/renderer.hpp>
#include <exd/render/render_graph.hpp>
#include <exd/render/camera.hpp>
#include <cstdio>

namespace exd::render {

std::unique_ptr<IRenderer> IRenderer::create(Backend backend) {
    switch (backend) {
        case Backend::Null:
            // Null backend implemented in null_renderer.cpp
            // For now, return nullptr to indicate headless mode
            std::fprintf(stderr, "[IRenderer] Null backend requested — headless mode\n");
            return nullptr;
        case Backend::OpenGL:
        case Backend::Vulkan:
        case Backend::WebGL:
            std::fprintf(stderr, "[IRenderer] Backend %d not yet implemented\n", (int)backend);
            return nullptr;
    }
    return nullptr;
}

} // namespace exd::render
