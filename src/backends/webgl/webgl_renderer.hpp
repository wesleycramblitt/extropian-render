#pragma once

#include <exd/render/renderer.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

namespace exd::render {

class WebGLRenderer : public IRenderer {
public:
    void initialize(void* window_handle) override;
    void shutdown() override;
    void resize(uint32_t width, uint32_t height) override;
    void begin_frame() override;
    void execute(const RenderGraph& graph, const Camera& camera) override;
    void end_frame() override;
    [[nodiscard]] std::string_view backend_name() const override;
    [[nodiscard]] std::string_view renderer_info() const override;

private:
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx_ = 0;
#endif
    uint32_t w_ = 800;
    uint32_t h_ = 600;
};

} // namespace exd::render
