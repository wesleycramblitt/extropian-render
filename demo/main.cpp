/// extropian-render demo — configuration guide
///
/// Every interaction feature is an independent system.  To use only a
/// subset in your own app, simply create only the systems you need:
///
///   CameraSystem       ← FPS fly camera (WASD + mouse)
///   PrimitiveMeshSystem ← generates GPU meshes from shape components
///   GridSystem         ← reference grid on XZ plane
///   CubeMapSystem      ← skybox cubemap loading
///   PolygonModeSystem  ← wireframe toggle
///   RenderSystem       ← all render passes (lambertian, mirror, highlight, etc.)
///   PickerSystem       ← CPU raycast picking (needed for selection)
///   SelectionSystem    ← click-to-select, hover tracking
///   GizmoSystem        ← translate/rotate/scale gizmo (needs SelectionSystem)
///
/// Omit any system and its feature disappears — no dependency chain
/// forces you to take the whole stack.

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
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/grid.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/geometry/primitives3d.hpp>
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <cstdio>

using namespace exd;

int main() {
    app::WindowDesc desc;
    desc.title = "extropian-render demo";
    app::Window window(desc);
    if (!window.is_valid()) {
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

    // Center — large cube
    auto center = reg.create("CenterCube");
    reg.emplace<render::Transform>(center, math::Vec3f{0, 1.5f, 0});
    reg.emplace<render::CubePrimitive>(center, 2.5f);
    reg.emplace<render::RenderTechnique_Lambertian>(center);

    // Generate shape meshes via extropian-geometry
    using namespace exd::geometry;
    uint32_t sphere_mesh = ctx.mesh_manager.create(
        generate_sphere_mesh(SphereGeometry{.radius=0.8f, .latitudeSegments=16, .longitudeSegments=32}));
    uint32_t cylinder_mesh = ctx.mesh_manager.create(
        generate_cylinder_mesh(CylinderGeometry{.radius=0.5f, .height=2.0f, .slices=32, .capped=true}));
    uint32_t cone_mesh = ctx.mesh_manager.create(
        generate_cone_mesh(ConeGeometry{.radius=0.8f, .height=2.0f, .slices=32, .capped=true}));

    auto add_shape = [&](const char* name, float x, float z, uint32_t mesh, float y=1.5f) {
        auto e = reg.create(name);
        reg.emplace<render::Transform>(e, math::Vec3f{x, y, z});
        reg.emplace<render::RenderableComponent>(e, mesh);
        reg.emplace<render::RenderTechnique_Lambertian>(e);
    };
    add_shape("Sphere",  4,  4, sphere_mesh);
    add_shape("Sphere", -4,  4, sphere_mesh);
    add_shape("Sphere",  4, -4, sphere_mesh);
    add_shape("Sphere", -4, -4, sphere_mesh);
    add_shape("Cylinder", -3, 0, cylinder_mesh);
    add_shape("Cylinder", -1, 0, cylinder_mesh);
    add_shape("Cylinder",  1, 0, cylinder_mesh);
    add_shape("Cone",  3, 0, cone_mesh, 1.0f);
    add_shape("Cone", -5,-5, cone_mesh, 1.0f);

    // Reflective cube
    auto mirror = reg.create("Mirror");
    reg.emplace<render::Transform>(mirror, math::Vec3f{0, 3.0f, 3});
    reg.emplace<render::CubePrimitive>(mirror, 1.5f);
    reg.emplace<render::RenderTechnique_Mirror>(mirror);

    // Skybox
    auto sky = reg.create("Skybox");
    auto& cm = reg.emplace<render::CubeMapComponent>(sky);
    cm.name = std::string(EXD_ASSETS_DIR) + "/cubemaps/default/cross.png";
    cm.cross_layout = true;
    reg.emplace<render::Transform>(sky);
    reg.emplace<render::RenderTechnique_CubeMap>(sky);

    mesh_sys.update(reg, 0.0);
    cubemap_sys.update(reg, 0.0);

    auto print_help = []() {
        std::printf("\n=== Demo Controls ===\n");
        std::printf("  Tab        toggle FPS/UI mode\n");
        std::printf("  FPS:       WASD fly, mouse look\n");
        std::printf("  UI:        click to select, 1/2/3 = T/R/S gizmo\n");
        std::printf("  Shift+click multi-select\n");
        std::printf("  G=grid, X=wireframe, H=help, Esc=quit\n\n");
    };
    print_help();

    uint64_t last = SDL_GetTicks();
    uint32_t prev_mouse = 0;
    int frame = 0;

    while (!window.should_close()) {
        uint64_t now = SDL_GetTicks();
        double dt = (now - last) / 1000.0;
        last = now; frame++;

        window.poll_events();
        if (window.was_key_released(SDL_SCANCODE_ESCAPE))
            window.close();

        if (window.was_key_released(SDL_SCANCODE_TAB)) {
            auto next = (window.input_mode == app::InputMode::FPS)
                ? app::InputMode::UI : app::InputMode::FPS;
            window.input_mode = next;
            window.set_input_mode(next);
        }

        cam_sys.update(reg, dt);
        window.reset_mouse_delta();
        grid_sys.update(reg, dt);
        poly_sys.update(reg, dt);
        mesh_sys.update(reg, dt);

        if (window.was_key_released(SDL_SCANCODE_1))
            gizmo.set_mode(render::interaction::GizmoMode::Translate);
        if (window.was_key_released(SDL_SCANCODE_2))
            gizmo.set_mode(render::interaction::GizmoMode::Rotate);
        if (window.was_key_released(SDL_SCANCODE_3))
            gizmo.set_mode(render::interaction::GizmoMode::Scale);
        if (window.was_key_released(SDL_SCANCODE_H))
            print_help();

        window.set_title(
            (window.input_mode == app::InputMode::FPS ? "FPS" : "UI")
            + std::string(" | ")
            + (gizmo.mode() == render::interaction::GizmoMode::Translate ? "T" :
               gizmo.mode() == render::interaction::GizmoMode::Rotate ? "R" : "S")
            + std::string(" gizmo | extropian-render demo"));

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
        }
        if (gizmo.is_dragging() && !held) gizmo.on_mouse_release();

        // ── Render ──────────────────────────────
        render_sys.update(reg, dt);

        if (window.grid_visible) {
            for (auto ge : reg.view<render::GridComponent, render::RenderableComponent>()) {
                auto& rc = reg.get<render::RenderableComponent>(ge);
                if (rc.mesh == 0) continue;
                for (auto e : reg.view<render::CameraComponent, render::Transform>()) {
                    auto& cc = reg.get<render::CameraComponent>(e);
                    auto& ct = reg.get<render::Transform>(e);
                    math::Vec3f fwd = (ct.rotation*math::Vec3f{0,0,-1}).normalized();
                    math::Vec3f up  = (ct.rotation*math::Vec3f{0,1,0}).normalized();
                    math::Mat4 v = math::Mat4::look_at(ct.position, ct.position+fwd, up);
                    math::Mat4 p = math::Mat4::perspective(cc.fov_y_radians,
                        (float)w/(float)h, cc.near_plane, cc.far_plane);
                    uint32_t prog = ctx.shader_manager.get_or_load("grid",
                        "shaders/opengl/gizmo/gizmo.vert",
                        "shaders/opengl/gizmo/gizmo.frag");
                    glUseProgram(prog);
                    glUniformMatrix4fv(glGetUniformLocation(prog,"u_view"),1,GL_FALSE,v.m);
                    glUniformMatrix4fv(glGetUniformLocation(prog,"u_proj"),1,GL_FALSE,p.m);
                    glUniformMatrix4fv(glGetUniformLocation(prog,"u_model"),1,GL_FALSE,
                        math::Mat4::identity().m);
                    glUniform4f(glGetUniformLocation(prog,"u_color"),0.25f,0.25f,0.25f,1.0f);
                    const auto* m = ctx.mesh_manager.bind(rc.mesh);
                    if (m->index_count > 0)
                        glDrawElements(m->topology, m->index_count, GL_UNSIGNED_INT, nullptr);
                    else
                        glDrawArrays(m->topology, 0, m->vertex_count);
                    glUseProgram(0);
                    break;
                }
                break;
            }
        }

        // ── Gizmo overlay (renders in both modes) ─
        for (auto e : reg.view<render::CameraComponent, render::Transform>()) {
                auto& cc = reg.get<render::CameraComponent>(e);
                auto& ct = reg.get<render::Transform>(e);
                math::Vec3f fwd = (ct.rotation*math::Vec3f{0,0,-1}).normalized();
                math::Vec3f up  = (ct.rotation*math::Vec3f{0,1,0}).normalized();
                math::Mat4 v = math::Mat4::look_at(ct.position, ct.position+fwd, up);
                math::Mat4 p = math::Mat4::perspective(cc.fov_y_radians,
                    (float)w/(float)h, cc.near_plane, cc.far_plane);
                gizmo.render(reg, v, p, ct.position);
            break;
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
