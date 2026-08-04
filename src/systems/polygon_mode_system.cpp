#include <exd/render/systems/polygon_mode_system.hpp>
#include <exd/core/macros.hpp>
#ifndef __EMSCRIPTEN__
#include <SDL3/SDL.h>
#endif
#include <exd/render/graphics/gl_loader.hpp>

namespace exd::render {

void PolygonModeSystem::update(exd::ecs::Registry&, double) {
#ifndef __EMSCRIPTEN__
    if (window_->was_key_released(SDL_SCANCODE_X)) {
        GL_CALL(glPolygonMode(GL_FRONT_AND_BACK,
                window_->wireframe ? GL_FILL : GL_LINE));
        if (window_->wireframe) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        window_->wireframe = !window_->wireframe;
    }
#else
    (void)window_;
#endif
}

} // namespace exd::render
