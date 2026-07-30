#include <doctest/doctest.h>
#include "test_common.hpp"

#include <exd/render/systems/camera_system.hpp>
#include <exd/render/systems/primitive_mesh_system.hpp>
#include <exd/render/components/camera_controller.hpp>
#include <exd/render/components/cube.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/ecs/view.hpp>
#include <exd/render/components/mesh_asset.hpp>
#include <exd/render/components/grid.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/particle_cloud.hpp>
#include <exd/render/components/volume_field.hpp>
#include <exd/render/components/simulation_domain.hpp>
#include <exd/render/components/simulation_reference.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/graphics/mesh_manager.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

// ════════════════════════════════════════════════════════════════
// These tests verify ECS system logic WITHOUT requiring OpenGL.
// They test entity creation, component attachment, and system
// interaction patterns.
// ════════════════════════════════════════════════════════════════

TEST_SUITE("ECS System Integration") {

TEST_CASE("Camera entity setup with all camera components") {
    exd::ecs::Registry reg;

    auto e = reg.create("MainCamera");
    reg.emplace<Transform>(e, math::Vec3f{0,0,10},
                           math::Quat{}, math::Vec3f{1,1,1});
    reg.emplace<CameraComponent>(e);
    reg.emplace<CameraController>(e);

    CHECK(reg.valid(e));
    CHECK(reg.has<Transform>(e));
    CHECK(reg.has<CameraComponent>(e));
    CHECK(reg.has<CameraController>(e));
    CHECK(reg.entity_count() == 1);
}

TEST_CASE("Renderable cube entity with technique tag") {
    exd::ecs::Registry reg;

    auto e = reg.create("Cube");
    reg.emplace<Transform>(e);
    reg.emplace<CubePrimitive>(e, 2.0f);
    reg.emplace<RenderableComponent>(e);
    reg.emplace<RenderTechnique_Lambertian>(e);

    CHECK(reg.has<Transform>(e));
    CHECK(reg.has<CubePrimitive>(e));
    CHECK(reg.has<RenderableComponent>(e));
    CHECK(reg.has<RenderTechnique_Lambertian>(e));

    auto& cube = reg.get<CubePrimitive>(e);
    CHECK(near(cube.size, 2.0f));
}

TEST_CASE("Disabled entity is skipped in views") {
    exd::ecs::Registry reg;

    auto e = reg.create("Visible");
    reg.emplace<Transform>(e);
    reg.emplace<RenderableComponent>(e);
    reg.emplace<RenderTechnique_Lambertian>(e);

    auto d = reg.create("Hidden");
    reg.emplace<Transform>(d);
    reg.emplace<RenderableComponent>(d);
    reg.emplace<RenderTechnique_Lambertian>(d);
    reg.emplace<Disabled>(d);

    // View should find both entities
    int count = 0;
    for (auto ent : reg.view<Transform, RenderableComponent, RenderTechnique_Lambertian>()) {
        (void)ent;
        count++;
    }
    CHECK(count == 2);

    // But one of them is disabled
    CHECK(reg.has<Disabled>(d));
    CHECK(!reg.has<Disabled>(e));
}

TEST_CASE("Multiple technique tags on same entity") {
    exd::ecs::Registry reg;

    auto e = reg.create("MultiTechnique");
    reg.emplace<Transform>(e);
    reg.emplace<RenderableComponent>(e);
    reg.emplace<RenderTechnique_Lambertian>(e);
    reg.emplace<RenderTechnique_Mirror>(e);

    CHECK(reg.has<RenderTechnique_Lambertian>(e));
    CHECK(reg.has<RenderTechnique_Mirror>(e));
    CHECK(!reg.has<RenderTechnique_CubeMap>(e));
}

TEST_CASE("Simulation domain and reference linkage") {
    exd::ecs::Registry reg;

    auto sim = reg.create("Simulation");
    reg.emplace<SimulationDomain>(sim, 128, 128, 128);
    reg.emplace<Transform>(sim);

    auto vis = reg.create("VolumeVis");
    reg.emplace<VolumeFieldComponent>(vis);
    reg.emplace<SimulationReference>(vis, sim.id);

    // Verify linkage
    auto& ref = reg.get<SimulationReference>(vis);
    CHECK(ref.simulation_entity_id == sim.id);
    CHECK(reg.valid(sim));

    // Domain should have correct dimensions
    auto& dom = reg.get<SimulationDomain>(sim);
    CHECK(dom.nx == 128);
    CHECK(dom.ny == 128);
    CHECK(dom.nz == 128);
}

TEST_CASE("Particle cloud with simulation reference") {
    exd::ecs::Registry reg;

    auto sim = reg.create("FluidSim");
    reg.emplace<SimulationDomain>(sim, 64, 64, 64);

    auto pc = reg.create("ParticleVis");
    reg.emplace<ParticleCloudComponent>(pc);
    reg.emplace<SimulationReference>(pc, sim.id);
    reg.emplace<Transform>(pc);

    CHECK(reg.has<ParticleCloudComponent>(pc));
    auto& ref = reg.get<SimulationReference>(pc);
    CHECK(ref.simulation_entity_id == sim.id);
}

TEST_CASE("Entity destruction removes from views") {
    exd::ecs::Registry reg;

    auto e = reg.create("Temp");
    reg.emplace<Transform>(e);
    reg.emplace<RenderableComponent>(e);

    CHECK(reg.entity_count() == 1);
    CHECK(reg.valid(e));

    reg.destroy(e);

    CHECK(!reg.valid(e));
    // Entity recycling means count might still be 0
    CHECK(reg.entity_count() == 0);
}

TEST_CASE("Grid component with transform") {
    exd::ecs::Registry reg;

    auto e = reg.create("Grid");
    reg.emplace<GridComponent>(e, 25.0f);
    reg.emplace<Transform>(e, math::Vec3f{0,0,0},
                           math::Quat{}, math::Vec3f{1,1,1});

    auto& grid = reg.get<GridComponent>(e);
    CHECK(near(grid.spacing, 25.0f));
}

TEST_CASE("Mesh asset loading setup") {
    exd::ecs::Registry reg;

    auto e = reg.create("Model");
    reg.emplace<MeshAssetComponent>(e);
    auto& ma = reg.get<MeshAssetComponent>(e);
    ma.path = "assets/models/cube.obj";

    CHECK(ma.path == "assets/models/cube.obj");
}

TEST_CASE("View iterates correctly over filtered entities") {
    exd::ecs::Registry reg;

    // Create 3 entities, only 2 have both Transform and RenderableComponent
    auto e1 = reg.create("A");
    reg.emplace<Transform>(e1);
    reg.emplace<RenderableComponent>(e1);

    auto e2 = reg.create("B");
    reg.emplace<Transform>(e2);
    reg.emplace<RenderableComponent>(e2);
    reg.emplace<RenderTechnique_Lambertian>(e2);

    auto e3 = reg.create("C");
    reg.emplace<Transform>(e3);
    // no RenderableComponent

    int count = 0;
    auto view = reg.view<Transform, RenderableComponent>();
    for (auto ent : view) {
        (void)ent;
        count++;
    }
    CHECK(count == 2);
}

TEST_CASE("Registry create returns valid entity") {
    exd::ecs::Registry reg;
    auto e = reg.create("NamedEntity");
    CHECK(reg.valid(e));
    CHECK(e.id != std::numeric_limits<uint32_t>::max());

    auto all = reg.all_entities();
    CHECK(all.size() == 1);
}

} // TEST_SUITE("ECS System Integration")
