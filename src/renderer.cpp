#include <exd/render/renderer.hpp>
#include <exd/render/render_graph.hpp>
#include <exd/render/camera.hpp>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include "backends/webgl/webgl_renderer.hpp"
#endif

namespace exd::render {

std::unique_ptr<IRenderer> IRenderer::create(Backend backend) {
    switch (backend) {
        case Backend::Null:
            std::fprintf(stderr, "[IRenderer] Null backend requested — headless mode\n");
            return nullptr;
        case Backend::OpenGL:
        case Backend::Vulkan:
            std::fprintf(stderr, "[IRenderer] Backend %d not yet implemented\n", (int)backend);
            return nullptr;
        case Backend::WebGL:
#ifdef __EMSCRIPTEN__
            return std::make_unique<WebGLRenderer>();
#else
            std::fprintf(stderr, "[IRenderer] WebGL backend requires Emscripten\n");
            return nullptr;
#endif
    }
    return nullptr;
}

} // namespace exd::render
