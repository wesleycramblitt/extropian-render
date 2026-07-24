#include <doctest/doctest.h>
#include "test_common.hpp"

#include <exd/render/components/transform.hpp>
#include <exd/render/components/camera_component.hpp>
#include <exd/render/components/camera_controller.hpp>
#include <exd/render/components/cube.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/components/grid.hpp>
#include <exd/render/components/mesh_asset.hpp>
#include <exd/render/components/particle_cloud.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/simulation_domain.hpp>
#include <exd/render/components/simulation_reference.hpp>
#include <exd/render/components/skew.hpp>
#include <exd/render/components/volume_field.hpp>
#include <exd/render/components/readonly.hpp>
#include <exd/ecs/registry.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

TEST_SUITE("Components") {

TEST_CASE("Transform default values") {
    Transform t;
    CHECK(near(t.position, math::Vec3f{0,0,0}));
    CHECK(near(t.scale,    math::Vec3f{1,1,1}));
    // Default quaternion is identity: w=1, x=y=z=0
    CHECK(near(t.rotation.w, 1.0f));
    CHECK(near(t.rotation.x, 0.0f));
    CHECK(near(t.rotation.y, 0.0f));
    CHECK(near(t.rotation.z, 0.0f));
}

TEST_CASE("CameraComponent default values") {
    CameraComponent c;
    CHECK(near(c.fov_y_radians, 1.047f));
    CHECK(near(c.near_plane,    0.1f));
    CHECK(near(c.far_plane,     1000.0f));
    CHECK(near(c.exposure,      1.0f));
}

TEST_CASE("CameraController default values") {
    CameraController cc;
    CHECK(near(cc.move_speed,        1.0f));
    CHECK(near(cc.sprint_mult,       2.0f));
    CHECK(near(cc.mouse_sensitivity, 0.0002f));
    CHECK(near(cc.yaw,               0.0f));
    CHECK(near(cc.pitch,             0.0f));
}

TEST_CASE("CubePrimitive default size") {
    CubePrimitive cp;
    CHECK(near(cp.size, 100.0f));
}

TEST_CASE("CubeMapComponent defaults") {
    CubeMapComponent cm;
    CHECK(cm.name.empty());
    CHECK(cm.cross_layout == true);
    CHECK(cm.texture_handle == 0);
    CHECK(cm.gl_cubemap == 0);
}

TEST_CASE("GridComponent defaults") {
    GridComponent g;
    CHECK(near(g.spacing, 50.0f));
}

TEST_CASE("MeshAssetComponent defaults") {
    MeshAssetComponent ma;
    CHECK(ma.path.empty());
}

TEST_CASE("RenderableComponent defaults") {
    RenderableComponent r;
    CHECK(r.mesh == 0);
}

TEST_CASE("Skew defaults") {
    Skew s;
    CHECK(near(s.shear, math::Vec3f{0,0,0}));
}

TEST_CASE("SimulationDomain defaults") {
    SimulationDomain sd;
    CHECK(sd.nx == 64);
    CHECK(sd.ny == 64);
    CHECK(sd.nz == 64);
}

TEST_CASE("SimulationReference defaults") {
    SimulationReference sr;
    CHECK(sr.simulation_entity_id == 0);
}

TEST_CASE("ParticleCloudComponent defaults") {
    ParticleCloudComponent pc;
    CHECK(pc.particle_count == 0);
    CHECK(pc.max_particles == 100000);
    CHECK(pc.positions.empty());
    CHECK(pc.colors.empty());
}

TEST_CASE("VolumeFieldComponent defaults") {
    VolumeFieldComponent vf;
    CHECK(vf.texture_handle == 0);
    CHECK(vf.interop_ready == false);
}

TEST_CASE("Marker components are default-constructible") {
    Disabled d;
    Selected s;
    ReadOnly r;
    RenderTechnique_Lambertian rtl;
    RenderTechnique_Mirror rtm;
    RenderTechnique_CubeMap rtc;
    RenderTechnique_Lit rli;
    // Just verify they compile and don't crash
    CHECK(true);
}

TEST_CASE("Component emplace and get via registry") {
    exd::ecs::Registry reg;
    auto e = reg.create("TestEntity");

    reg.emplace<Transform>(e, math::Vec3f{1,2,3});
    reg.emplace<CameraComponent>(e);

    CHECK(reg.has<Transform>(e));
    CHECK(reg.has<CameraComponent>(e));
    CHECK(!reg.has<Skew>(e));

    auto& t = reg.get<Transform>(e);
    CHECK(near(t.position, math::Vec3f{1,2,3}));

    auto& c = reg.get<CameraComponent>(e);
    CHECK(near(c.fov_y_radians, 1.047f));
}

TEST_CASE("Component remove") {
    exd::ecs::Registry reg;
    auto e = reg.create("Test");

    reg.emplace<Skew>(e);
    CHECK(reg.has<Skew>(e));

    reg.remove<Skew>(e);
    CHECK(!reg.has<Skew>(e));
}

TEST_CASE("Multiple components on one entity") {
    exd::ecs::Registry reg;
    auto e = reg.create("MultiComponent");

    reg.emplace<Transform>(e);
    reg.emplace<RenderableComponent>(e);
    reg.emplace<Disabled>(e);

    CHECK(reg.has<Transform>(e));
    CHECK(reg.has<RenderableComponent>(e));
    CHECK(reg.has<Disabled>(e));
}

} // TEST_SUITE("Components")
