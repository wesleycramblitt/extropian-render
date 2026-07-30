#include "webgl_renderer.hpp"
#include <exd/render/render_graph.hpp>
#include <exd/render/camera.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstdio>
#endif

namespace exd::render {

#ifdef __EMSCRIPTEN__

// ════════════════════════════════════════════════════════════════════
// WebGLRenderer (Emscripten / WebGL 2.0)
// ════════════════════════════════════════════════════════════════════

void WebGLRenderer::initialize(void* window_handle) {
    const char* canvas_id = window_handle
        ? static_cast<const char*>(window_handle)
        : "#canvas";

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = true;
    attrs.depth = true;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.enableExtensionsByDefault = true;
    attrs.premultipliedAlpha = true;
    attrs.preserveDrawingBuffer = false;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

    ctx_ = emscripten_webgl_create_context(canvas_id, &attrs);
    if (ctx_ <= 0) {
        std::fprintf(stderr, "[WebGLRenderer] Failed to create WebGL 2.0 context\n");
        return;
    }

    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(ctx_);
    if (res != EMSCRIPTEN_RESULT_SUCCESS) {
        std::fprintf(stderr, "[WebGLRenderer] Failed to make context current\n");
        emscripten_webgl_destroy_context(ctx_);
        ctx_ = 0;
        return;
    }

    // Get initial canvas dimensions
    double dw = 0.0, dh = 0.0;
    emscripten_get_element_css_size(canvas_id, &dw, &dh);
    w_ = static_cast<uint32_t>(dw > 0 ? dw : 800);
    h_ = static_cast<uint32_t>(dh > 0 ? dh : 600);

    emscripten_set_canvas_element_size(canvas_id, static_cast<int>(w_), static_cast<int>(h_));
    glViewport(0, 0, static_cast<GLsizei>(w_), static_cast<GLsizei>(h_));

    std::fprintf(stderr, "[WebGLRenderer] WebGL 2.0 context created (%ux%u)\n", w_, h_);
}

void WebGLRenderer::shutdown() {
    if (ctx_ != 0) {
        emscripten_webgl_destroy_context(ctx_);
        ctx_ = 0;
    }
}

void WebGLRenderer::resize(uint32_t width, uint32_t height) {
    w_ = width;
    h_ = height;
    emscripten_set_canvas_element_size("#canvas", static_cast<int>(w_), static_cast<int>(h_));
    glViewport(0, 0, static_cast<GLsizei>(w_), static_cast<GLsizei>(h_));
}

void WebGLRenderer::begin_frame() {
    glViewport(0, 0, static_cast<GLsizei>(w_), static_cast<GLsizei>(h_));
    glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void WebGLRenderer::execute(const RenderGraph& /*graph*/, const Camera& /*camera*/) {
    // Stub: RenderSystem handles execution directly for now.
}

void WebGLRenderer::end_frame() {
    // Browser handles buffer swap via requestAnimationFrame.
    // Nothing to do here.
}

std::string_view WebGLRenderer::backend_name() const {
    return "WebGL";
}

std::string_view WebGLRenderer::renderer_info() const {
    return "WebGL 2.0 (Emscripten)";
}

#else // !__EMSCRIPTEN__

// ════════════════════════════════════════════════════════════════════
// WebGLRenderer (Desktop — empty stub; factory dispatch is at IRenderer level)
// ════════════════════════════════════════════════════════════════════

void WebGLRenderer::initialize(void*) {}
void WebGLRenderer::shutdown() {}
void WebGLRenderer::resize(uint32_t, uint32_t) {}
void WebGLRenderer::begin_frame() {}
void WebGLRenderer::execute(const RenderGraph&, const Camera&) {}
void WebGLRenderer::end_frame() {}
std::string_view WebGLRenderer::backend_name() const { return "WebGL"; }
std::string_view WebGLRenderer::renderer_info() const { return "WebGL 2.0 (Emscripten)"; }

#endif // __EMSCRIPTEN__

} // namespace exd::render
