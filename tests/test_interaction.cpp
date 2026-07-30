#include <doctest/doctest.h>
#include "test_common.hpp"

#include <SDL3/SDL.h>
#include <exd/render/graphics/gl_loader.hpp>

#include <exd/render/interaction/picker.hpp>
#include <exd/render/interaction/selection.hpp>
#include <exd/render/interaction/gizmo.hpp>

#include <exd/render/components/transform.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/hovered.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/components/cube.hpp>

#include <exd/render/graphics/techniques/highlight_technique.hpp>
#include <exd/render/graphics/mesh_manager.hpp>
#include <exd/render/systems/primitive_mesh_system.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

// ════════════════════════════════════════════════════════════════
// Shared GL fixture for interaction tests
// ════════════════════════════════════════════════════════════════

struct InteractionGLFixture {
    SDL_Window*   window  = nullptr;
    SDL_GLContext gl_ctx  = nullptr;
    GraphicsContext ctx;

    InteractionGLFixture() {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        window = SDL_CreateWindow("test", 800, 600,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (!window) return;

        gl_ctx = SDL_GL_CreateContext(window);
        if (!gl_ctx) return;

        SDL_GL_MakeCurrent(window, gl_ctx);
        gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    }

    ~InteractionGLFixture() {
        if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool ok() const { return window && gl_ctx; }

    uint32_t create_cube_mesh(float size = 1.0f) {
        Mesh mesh;
        float h = size * 0.5f;
        struct Face { math::Vec3f n, v0, v1, v2, v3; };
        Face faces[6] = {
            {{1,0,0}, {h,-h,-h},{h,h,-h},{h,h,h},{h,-h,h}},
            {{-1,0,0},{-h,-h,h},{-h,h,h},{-h,h,-h},{-h,-h,-h}},
            {{0,1,0},{-h,h,-h},{-h,h,h},{h,h,h},{h,h,-h}},
            {{0,-1,0},{-h,-h,h},{-h,-h,-h},{h,-h,-h},{h,-h,h}},
            {{0,0,1},{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}},
            {{0,0,-1},{-h,-h,-h},{-h,h,-h},{h,h,-h},{h,-h,-h}},
        };
        for (auto& f : faces) {
            uint32_t start = mesh.vertices.size();
            mesh.vertices.push_back({f.v0, f.n});
            mesh.vertices.push_back({f.v1, f.n});
            mesh.vertices.push_back({f.v2, f.n});
            mesh.vertices.push_back({f.v3, f.n});
            mesh.indices.insert(mesh.indices.end(),
                {start+0,start+1,start+2,start+0,start+2,start+3});
        }
        return ctx.mesh_manager.create(mesh);
    }
};

// ════════════════════════════════════════════════════════════════
// PickerSystem tests
// ════════════════════════════════════════════════════════════════

TEST_SUITE("PickerSystem") {

TEST_CASE_FIXTURE(InteractionGLFixture, "pick on cube entity returns entity") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    uint32_t mesh_id = create_cube_mesh(1.0f);

    auto e = reg.create("Cube");
    reg.emplace<Transform>(e, math::Vec3f{0,0,0});
    reg.emplace<RenderableComponent>(e, mesh_id);

    PickerSystem picker(ctx.mesh_manager);

    // Camera looking at origin from Z=5
    auto hit = picker.pick(reg, {0,0,5}, {0,0,-1}, {0,1,0},
                           1.047f, 800.0f/600.0f,
                           400, 300, 800, 600);  // center of screen

    CHECK(hit.has_value());
    CHECK(hit->id == e.id);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "pick misses when no entity at position") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    create_cube_mesh(1.0f);

    auto e = reg.create("Cube");
    reg.emplace<Transform>(e, math::Vec3f{00,100,0});  // far off-screen
    reg.emplace<RenderableComponent>(e, 1);

    PickerSystem picker(ctx.mesh_manager);

    auto hit = picker.pick(reg, {0,0,5}, {0,0,-1}, {0,1,0},
                           1.047f, 800.0f/600.0f,
                           400, 300, 800, 600);
    CHECK(!hit.has_value());
}

TEST_CASE_FIXTURE(InteractionGLFixture, "pick ignores disabled entities") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    uint32_t mesh_id = create_cube_mesh(1.0f);

    auto e = reg.create("DisabledCube");
    reg.emplace<Transform>(e, math::Vec3f{0,0,0});
    reg.emplace<RenderableComponent>(e, mesh_id);
    reg.emplace<Disabled>(e);

    PickerSystem picker(ctx.mesh_manager);

    auto hit = picker.pick(reg, {0,0,5}, {0,0,-1}, {0,1,0},
                           1.047f, 800.0f/600.0f,
                           400, 300, 800, 600);
    CHECK(!hit.has_value());
}

TEST_CASE_FIXTURE(InteractionGLFixture, "pick picks closest entity") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    uint32_t mesh_id = create_cube_mesh(1.0f);

    // Camera at z=5 looking toward -Z.
    // near entity at z=2 is closer than far entity at z=0.
    auto near_e  = reg.create("NearCube");
    auto far_e   = reg.create("FarCube");
    reg.emplace<Transform>(near_e, math::Vec3f{0,0,2});
    reg.emplace<Transform>(far_e,  math::Vec3f{0,0,0});
    reg.emplace<RenderableComponent>(near_e, mesh_id);
    reg.emplace<RenderableComponent>(far_e, mesh_id);

    PickerSystem picker(ctx.mesh_manager);

    auto hit = picker.pick(reg, {0,0,5}, {0,0,-1}, {0,1,0},
                           1.047f, 800.0f/600.0f,
                           400, 300, 800, 600);

    CHECK(hit.has_value());
    CHECK(hit->id == near_e.id);
}

} // TEST_SUITE("PickerSystem")

// ════════════════════════════════════════════════════════════════
// HighlightTechnique tests
// ════════════════════════════════════════════════════════════════

TEST_SUITE("HighlightTechnique") {

TEST_CASE_FIXTURE(InteractionGLFixture, "bind/unbind restores GL state") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    HighlightTechnique highlight(ctx);

    math::Mat4 view = math::Mat4::identity();
    math::Mat4 proj = math::Mat4::identity();

    highlight.bind(view, proj);
    highlight.unbind();

    // After unbind, polygon mode should be restored to FILL
    GLint mode[2];
    glGetIntegerv(GL_POLYGON_MODE, mode);
    CHECK(mode[0] == GL_FILL);

    // Depth mask should be restored
    GLint depth_mask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &depth_mask);
    CHECK(depth_mask == GL_TRUE);

    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "draw with mesh_handle 0 is safe") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    HighlightTechnique highlight(ctx);
    math::Mat4 view = math::Mat4::identity();
    math::Mat4 proj = math::Mat4::identity();

    highlight.bind(view, proj);
    highlight.draw(0, math::Mat4::identity());
    highlight.unbind();

    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "highlight renders wireframe on cube") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    uint32_t mesh_id = create_cube_mesh(1.0f);
    REQUIRE(mesh_id > 0);

    HighlightTechnique highlight(ctx);

    math::Mat4 view = math::Mat4::look_at({0,1,4}, {0,0,0}, {0,1,0});
    math::Mat4 proj = math::Mat4::perspective(1.047f, 800.0f/600.0f, 0.1f, 100.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    highlight.bind(view, proj);
    highlight.draw(mesh_id, math::Mat4::identity());
    highlight.unbind();

    SDL_GL_SwapWindow(window);
    CHECK(glGetError() == GL_NO_ERROR);
}

} // TEST_SUITE("HighlightTechnique")

// ════════════════════════════════════════════════════════════════
// GizmoSystem tests
// ════════════════════════════════════════════════════════════════

TEST_SUITE("GizmoSystem") {

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo does not render with no selection") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    GizmoSystem gizmo(ctx);

    math::Mat4 view = math::Mat4::look_at({0,1,4}, {0,0,0}, {0,1,0});
    math::Mat4 proj = math::Mat4::perspective(1.047f, 800.0f/600.0f, 0.1f, 100.0f);

    // Should not crash with no selection
    gizmo.render(reg, view, proj, {0,1,4});
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo renders with selection") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    create_cube_mesh(1.0f);

    auto e = reg.create("SelectedCube");
    reg.emplace<Transform>(e, math::Vec3f{0,0,0});
    reg.emplace<Selected>(e);

    GizmoSystem gizmo(ctx);

    math::Mat4 view = math::Mat4::look_at({0,1,4}, {0,0,0}, {0,1,0});
    math::Mat4 proj = math::Mat4::perspective(1.047f, 800.0f/600.0f, 0.1f, 100.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gizmo.render(reg, view, proj, {0,1,4});

    SDL_GL_SwapWindow(window);

    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo mode toggle works") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    GizmoSystem gizmo(ctx);

    CHECK(gizmo.mode() == interaction::GizmoMode::Translate);

    gizmo.set_mode(interaction::GizmoMode::Rotate);
    CHECK(gizmo.mode() == interaction::GizmoMode::Rotate);

    gizmo.set_mode(interaction::GizmoMode::Scale);
    CHECK(gizmo.mode() == interaction::GizmoMode::Scale);

    gizmo.set_mode(interaction::GizmoMode::Translate);
    CHECK(gizmo.mode() == interaction::GizmoMode::Translate);
}

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo not dragging by default") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    GizmoSystem gizmo(ctx);
    CHECK(!gizmo.is_dragging());
}

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo click away from gizmo returns false") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    create_cube_mesh(1.0f);

    auto e = reg.create("Entity");
    reg.emplace<Transform>(e, math::Vec3f{0,0,0});
    reg.emplace<Selected>(e);

    GizmoSystem gizmo(ctx);

    // Click far away from the gizmo (which is at origin)
    bool captured = gizmo.on_mouse_press(reg,
        {0,1,4}, {0,0,-1}, {0,1,0}, 1.047f, 800.0f/600.0f,
        0, 0, 800, 600);  // top-left corner

    CHECK(!captured);
    CHECK(!gizmo.is_dragging());
}

TEST_CASE_FIXTURE(InteractionGLFixture, "gizmo renders all three modes") {
    if (!ok()) { MESSAGE("Skipping: no GL context"); return; }

    exd::ecs::Registry reg;
    create_cube_mesh(1.0f);

    auto e = reg.create("SelectedCube");
    reg.emplace<Transform>(e, math::Vec3f{0,0,0});
    reg.emplace<Selected>(e);

    GizmoSystem gizmo(ctx);

    math::Mat4 view = math::Mat4::look_at({0,1,4}, {0,0,0}, {0,1,0});
    math::Mat4 proj = math::Mat4::perspective(1.047f, 800.0f/600.0f, 0.1f, 100.0f);

    using namespace interaction;

    // Translate mode
    gizmo.set_mode(GizmoMode::Translate);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gizmo.render(reg, view, proj, {0,1,4});
    CHECK(glGetError() == GL_NO_ERROR);

    // Rotate mode
    gizmo.set_mode(GizmoMode::Rotate);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gizmo.render(reg, view, proj, {0,1,4});
    CHECK(glGetError() == GL_NO_ERROR);

    // Scale mode
    gizmo.set_mode(GizmoMode::Scale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gizmo.render(reg, view, proj, {0,1,4});
    CHECK(glGetError() == GL_NO_ERROR);
}

} // TEST_SUITE("GizmoSystem")
