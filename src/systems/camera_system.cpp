#include <exd/render/systems/camera_system.hpp>
#include <exd/render/components/camera_component.hpp>
#include <exd/render/components/camera_controller.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/ecs/view.hpp>
#include <exd/core/window_state.hpp>
#include <exd/core/macros.hpp>
#ifndef __EMSCRIPTEN__
#include <SDL3/SDL.h>
#endif
#include <algorithm>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// CameraSystem
// ════════════════════════════════════════════════════════════════════

void CameraSystem::update(exd::ecs::Registry& registry, double dt) {
#ifndef __EMSCRIPTEN__
    using namespace exd::math;
    using exd::core::InputMode;
    if (window_->input_mode != InputMode::FPS) return;
    if (!window_->keyboard_state) return;

    for (auto e : registry.view<CameraController, CameraComponent, Transform>()) {
        auto& cc = registry.get<CameraController>(e);
        float dx = -window_->mouse_rel_x;
        float dy = -window_->mouse_rel_y;

        cc.yaw   += dx * cc.mouse_sensitivity;
        cc.pitch += dy * cc.mouse_sensitivity;
        cc.pitch = std::clamp(cc.pitch, -1.55f, 1.55f);
        if (cc.yaw > 6.283f)  cc.yaw -= 6.283f;
        if (cc.yaw < -6.283f) cc.yaw += 6.283f;

        Vec3f world_up{0.0f, 1.0f, 0.0f};
        Quat q_yaw = Quat::from_axis_angle(world_up, cc.yaw);
        Vec3f local_right = (q_yaw * Vec3f{1.0f, 0.0f, 0.0f}).normalized();
        Quat q_pitch = Quat::from_axis_angle(local_right, cc.pitch);
        auto& xform = registry.get<Transform>(e);
        xform.rotation = (q_pitch * q_yaw).norm();

        Vec3f cam_fwd = (xform.rotation * Vec3f{0.0f, 0.0f, -1.0f}).normalized();
        Vec3f front = (cam_fwd - world_up * cam_fwd.dot(world_up)).normalized();
        float s = cc.move_speed * dt *
            (window_->keyboard_state[SDL_SCANCODE_LSHIFT] ? cc.sprint_mult : 1.0f);
        Vec3f move{0.0f, 0.0f, 0.0f};
        auto& ks = window_->keyboard_state;
        if (ks[SDL_SCANCODE_W]) move = move + front * s;
        if (ks[SDL_SCANCODE_S]) move = move - front * s;
        if (ks[SDL_SCANCODE_A]) move = move - local_right * s;
        if (ks[SDL_SCANCODE_D]) move = move + local_right * s;
        if (ks[SDL_SCANCODE_Q]) move = move - world_up * s;
        if (ks[SDL_SCANCODE_E]) move = move + world_up * s;
        xform.position = xform.position + move;

        break;
    }
    window_->mouse_rel_x = 0;
    window_->mouse_rel_y = 0;
#else
    (void)registry; (void)dt;
#endif
}

} // namespace exd::render
