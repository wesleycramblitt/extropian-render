#include <exd/render/systems/polygon_mode_system.hpp>
#include <exd/core/macros.hpp>
#include <glad/gl.h>
#include <SDL3/SDL.h>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// PolygonModeSystem
// ════════════════════════════════════════════════════════════════════

void PolygonModeSystem::update(exd::ecs::Registry&, double) {
    if (window_->was_key_released(SDL_SCANCODE_X)) {
        GL_CALL(glPolygonMode(GL_FRONT_AND_BACK,
                window_->wireframe ? GL_FILL : GL_LINE));
        if (window_->wireframe) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        window_->wireframe = !window_->wireframe;
    }
}

} // namespace exd::render
