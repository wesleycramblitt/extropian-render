/// extropian-render comprehensive demo
///
/// Scene: variety of shapes (cubes, spheres, cylinders, cones),
/// reflective surface, skybox, reference grid.  Full interaction
/// pipeline: picking, selection, highlight, and all three gizmo modes.
///
/// ── Controls ─────────────────────────────────────
///   Tab          toggle FPS camera / UI interaction mode
///
///   FPS mode:    WASD fly, mouse look
///
///   UI mode:     cursor visible
///     1/2/3      Translate / Rotate / Scale gizmo
///     Left-click select entity → orange wireframe highlight
///     Drag gizmo arrows/rings/boxes to transform selection
///     Shift+click add/remove from multi-selection
///     G          toggle reference grid
///     X          toggle wireframe
///     Esc        quit

#ifndef EXD_ASSETS_DIR
#define EXD_ASSETS_DIR "../extropian-assets"
#endif

#include <exd/app/window.hpp>
#include <exd/app/input_mode.hpp>
#include <exd/ecs/registry.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/systems/render_system.hpp>
#include <exd/render/systems/camera_system.hpp>
#include <exd/render/systems/primitive_mesh_system.hpp>
#include <exd/render/systems/grid_system.hpp>
#include <exd/render/systems/cubemap_system.hpp>
#include <exd/render/systems/polygon_mode_system.hpp>
#include <exd/render/interaction/picker.hpp>
#include <exd/render/interaction/selection.hpp>
#include <exd/render/interaction/gizmo.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/components/camera_component.hpp>
#include <exd/render/components/camera_controller.hpp>
#include <exd/render/components/cube.hpp>
#include <exd/render/components/sphere.hpp>
#include <exd/render/components/cylinder.hpp>
#include <exd/render/components/cone.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/grid.hpp>
#include <exd/render/components/selected.hpp>
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <cstdio>
#include <string>

using namespace exd;

int main() {
    app::Window window;
    if (!window.sdl_window || !window.gl_context) {
        std::fprintf(stderr, "FATAL: window creation failed\n");
        return 1;
    }
    std::printf("OpenGL %s | %s\n",
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    int w, h; float aspect;
    window.get_dimensions(w, h, aspect);

    ecs::Registry reg;
    render::GraphicsContext ctx;

    // Systems
    render::CameraSystem       cam_sys(&window);
    render::PrimitiveMeshSystem mesh_sys(ctx, &window);
    render::GridSystem         grid_sys(ctx, &window);
    render::CubeMapSystem      cubemap_sys(ctx, &window);
    render::PolygonModeSystem  poly_sys(&window);
    render::RenderSystem       render_sys(ctx, &window);
    render::PickerSystem       picker(ctx.mesh_manager);
    render::SelectionSystem    selection;
    render::GizmoSystem        gizmo(ctx);

    // ── Scene ────────────────────────────────────
    auto cam = reg.create("Camera");
    reg.emplace<render::Transform>(cam, math::Vec3f{0, 4, 14});
    reg.emplace<render::CameraComponent>(cam);
    reg.emplace<render::CameraController>(cam);

    auto grid = reg.create("Grid");
    reg.emplace<render::GridComponent>(grid, 50.0f);
    reg.emplace<render::Transform>(grid);

    // Center — large cube (easy click target)
    auto center = reg.create("CenterCube");
    reg.emplace<render::Transform>(center, math::Vec3f{0, 1.5f, 0});
    reg.emplace<render::CubePrimitive>(center, 2.5f);
    reg.emplace<render::RenderTechnique_Lambertian>(center);

    // Spheres — ring of 4
    auto s1 = reg.create("Sphere1");
    reg.emplace<render::Transform>(s1, math::Vec3f{ 4, 1.5f,  4});
    reg.emplace<render::SpherePrimitive>(s1, 0.8f, 32);
    reg.emplace<render::RenderTechnique_Lambertian>(s1);

    auto s2 = reg.create("Sphere2");
    reg.emplace<render::Transform>(s2, math::Vec3f{-4, 1.5f,  4});
    reg.emplace<render::SpherePrimitive>(s2, 0.6f, 24);
    reg.emplace<render::RenderTechnique_Lambertian>(s2);

    auto s3 = reg.create("Sphere3");
    reg.emplace<render::Transform>(s3, math::Vec3f{ 4, 1.5f, -4});
    reg.emplace<render::SpherePrimitive>(s3, 0.7f, 32);
    reg.emplace<render::RenderTechnique_Lambertian>(s3);

    auto s4 = reg.create("Sphere4");
    reg.emplace<render::Transform>(s4, math::Vec3f{-4, 1.5f, -4});
    reg.emplace<render::SpherePrimitive>(s4, 1.0f, 40);
    reg.emplace<render::RenderTechnique_Lambertian>(s4);

    // Cylinders — row of 3
    auto c1 = reg.create("Cylinder1");
    reg.emplace<render::Transform>(c1, math::Vec3f{-3, 1.5f, 0});
    reg.emplace<render::CylinderPrimitive>(c1, 0.4f, 2.0f, 32);
    reg.emplace<render::RenderTechnique_Lambertian>(c1);

    auto c2 = reg.create("Cylinder2");
    reg.emplace<render::Transform>(c2, math::Vec3f{-1, 1.5f, 0});
    reg.emplace<render::CylinderPrimitive>(c2, 0.5f, 1.5f, 32);
    reg.emplace<render::RenderTechnique_Lambertian>(c2);

    auto c3 = reg.create("Cylinder3");
    reg.emplace<render::Transform>(c3, math::Vec3f{ 1, 1.5f, 0});
    reg.emplace<render::CylinderPrimitive>(c3, 0.3f, 2.5f, 24);
    reg.emplace<render::RenderTechnique_Lambertian>(c3);

    // Cones — trio
    auto cn1 = reg.create("Cone1");
    reg.emplace<render::Transform>(cn1, math::Vec3f{ 3, 1.0f, 0});
    reg.emplace<render::ConePrimitive>(cn1, 0.8f, 2.0f, 32);
    reg.emplace<render::RenderTechnique_Lambertian>(cn1);

    auto cn2 = reg.create("Cone2");
    reg.emplace<render::Transform>(cn2, math::Vec3f{-5, 1.0f, -5});
    reg.emplace<render::ConePrimitive>(cn2, 0.6f, 1.5f, 24);
    reg.emplace<render::RenderTechnique_Lambertian>(cn2);

    // Reflective cube (mirror technique — reflects skybox)
    auto mirror = reg.create("Mirror");
    reg.emplace<render::Transform>(mirror, math::Vec3f{0, 3.0f, 3});
    reg.emplace<render::CubePrimitive>(mirror, 1.5f);
    reg.emplace<render::RenderTechnique_Mirror>(mirror);

    // Floating reflective sphere
    auto mir_sph = reg.create("MirrorSphere");
    reg.emplace<render::Transform>(mir_sph, math::Vec3f{5, 3.0f, 5});
    reg.emplace<render::SpherePrimitive>(mir_sph, 0.8f, 32);
    reg.emplace<render::RenderTechnique_Mirror>(mir_sph);

    // Skybox
    auto sky = reg.create("Skybox");
    auto& cm = reg.emplace<render::CubeMapComponent>(sky);
    cm.name = std::string(EXD_ASSETS_DIR) + "/cubemaps/default/cross.png";
    cm.cross_layout = true;
    reg.emplace<render::Transform>(sky);
    reg.emplace<render::RenderTechnique_CubeMap>(sky);

    mesh_sys.update(reg, 0.0);
    cubemap_sys.update(reg, 0.0);

    std::printf("\n=== extropian-render demo ===\n");
    std::printf("Entities: %zu  |  Assets: %s\n",
                reg.entity_count(), EXD_ASSETS_DIR);
    std::printf("\nControls:\n");
    std::printf("  Tab       toggle FPS/UI mode\n");
    std::printf("  FPS:      WASD fly, mouse look\n");
    std::printf("  UI:       1/2/3 = Translate/Rotate/Scale gizmo\n");
    std::printf("            Click = select (orange highlight)\n");
    std::printf("            Drag gizmo = transform\n");
    std::printf("  G         toggle grid\n");
    std::printf("  X         toggle wireframe\n");
    std::printf("  Esc       quit\n\n");

    // ── Main loop ────────────────────────────────
    uint64_t last = SDL_GetTicks();
    uint32_t prev_mouse = 0;
    bool running = true;
    int frame = 0;

    while (running) {
        uint64_t now = SDL_GetTicks();
        double dt = (now - last) / 1000.0;
        last = now; frame++;

        window.poll_events();
        if (window.should_close) running = false;
        if (window.was_key_released(SDL_SCANCODE_ESCAPE)) running = false;

        if (window.was_key_released(SDL_SCANCODE_TAB)) {
            window.input_mode = (window.input_mode == app::InputMode::FPS)
                ? app::InputMode::UI : app::InputMode::FPS;
            window.set_input_mode(window.input_mode);
        }

        cam_sys.update(reg, dt);
        window.event_state.mouse_rel_x = 0;
        window.event_state.mouse_rel_y = 0;
        grid_sys.update(reg, dt);
        poly_sys.update(reg, dt);
        mesh_sys.update(reg, dt);

        // ── UI interaction ───────────────────────
        float mx, my;
        uint32_t btn = SDL_GetMouseState(&mx, &my);
        bool click = (btn & SDL_BUTTON_LMASK) && !(prev_mouse & SDL_BUTTON_LMASK);
        bool held  = (btn & SDL_BUTTON_LMASK);
        bool shift = window.keyboard_state && window.keyboard_state[SDL_SCANCODE_LSHIFT];
        prev_mouse = btn;

        if (window.input_mode == app::InputMode::UI) {
            for (auto e : reg.view<render::CameraComponent, render::Transform>()) {
                auto& cc = reg.get<render::CameraComponent>(e);
                auto& ct = reg.get<render::Transform>(e);
                math::Vec3f fwd = (ct.rotation * math::Vec3f{0,0,-1}).normalized();
                math::Vec3f up  = (ct.rotation * math::Vec3f{0,1,0}).normalized();
                float ar = (float)w / (float)h;

                if (gizmo.is_dragging()) {
                    if (!held) gizmo.on_mouse_release();
                    else gizmo.on_mouse_drag(reg, ct.position, fwd, up,
                        cc.fov_y_radians, ar, mx, my, (float)w, (float)h);
                } else if (click) {
                    bool hit = gizmo.on_mouse_press(reg, ct.position, fwd, up,
                        cc.fov_y_radians, ar, mx, my, (float)w, (float)h);
                    if (!hit) {
                        auto pick = picker.pick(reg, ct.position, fwd, up,
                            cc.fov_y_radians, ar, mx, my, (float)w, (float)h);
                        selection.handle_click(reg, pick, shift);
                    }
                }
                break;
            }

            if (window.was_key_released(SDL_SCANCODE_1))
                gizmo.set_mode(render::interaction::GizmoMode::Translate);
            if (window.was_key_released(SDL_SCANCODE_2))
                gizmo.set_mode(render::interaction::GizmoMode::Rotate);
            if (window.was_key_released(SDL_SCANCODE_3))
                gizmo.set_mode(render::interaction::GizmoMode::Scale);
        }

        if (gizmo.is_dragging() && !held) gizmo.on_mouse_release();

        // ── Render ──────────────────────────────
        render_sys.update(reg, dt);

        if (window.input_mode == app::InputMode::UI) {
            for (auto e : reg.view<render::CameraComponent, render::Transform>()) {
                auto& cc = reg.get<render::CameraComponent>(e);
                auto& ct = reg.get<render::Transform>(e);
                math::Vec3f fwd = (ct.rotation * math::Vec3f{0,0,-1}).normalized();
                math::Vec3f up  = (ct.rotation * math::Vec3f{0,1,0}).normalized();
                math::Mat4 v = math::Mat4::look_at(ct.position, ct.position+fwd, up);
                math::Mat4 p = math::Mat4::perspective(cc.fov_y_radians,
                    (float)w/(float)h, cc.near_plane, cc.far_plane);
                gizmo.render(reg, v, p, ct.position);
                break;
            }
        }

        window.swap_buffers();
        window.get_dimensions(w, h, aspect);

        if (frame % 60 == 0) {
            const char* names[] = {"Translate","Rotate","Scale"};
            std::printf("[%4d] FPS=%.0f  sel=%d  gizmo=%s  mode=%s\n",
                frame, 1.0/dt, selection.selection_count(reg),
                names[static_cast<int>(gizmo.mode())],
                window.input_mode == app::InputMode::FPS ? "FPS" : "UI");
        }
    }

    std::printf("[demo] Shutdown — %d frames rendered\n", frame);
    return 0;
}
