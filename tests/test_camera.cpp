#include <doctest/doctest.h>
#include "test_common.hpp"

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

TEST_SUITE("Camera") {

TEST_CASE("Default camera looks down -Z") {
    Camera cam;
    CHECK(near(cam.position, math::Vec3f{0,0,0}));
    CHECK(near(cam.forward,  math::Vec3f{0,0,-1}));
    CHECK(near(cam.up,       math::Vec3f{0,1,0}));
}

TEST_CASE("view_matrix produces sensible result") {
    Camera cam;
    math::Mat4 view = cam.view_matrix();

    // The view matrix should transform world origin to camera space.
    // With camera at origin looking at (0,0,-1), the view matrix
    // should be near-identity (looking down -Z).
    // Check that it's not all zeros.
    bool has_nonzero = false;
    for (int i = 0; i < 16; ++i)
        if (!near(view.m[i], 0.0f)) { has_nonzero = true; break; }
    CHECK(has_nonzero);

    // Since position is (0,0,0) and we look at (0,0,-1) with up (0,1,0),
    // the view matrix should be near-identity.
    math::Mat4 identity = math::Mat4::identity();
    CHECK(near_mat4(view, identity, 1e-3f));
}

TEST_CASE("view_matrix with offset position is non-identity") {
    Camera cam;
    cam.position = math::Vec3f{0, 0, 5};  // moved forward
    math::Mat4 view = cam.view_matrix();

    // Camera moved forward should produce a different view matrix
    // than the default position (identity view)
    Camera default_cam;
    math::Mat4 default_view = default_cam.view_matrix();

    bool differs = false;
    for (int i = 0; i < 16; ++i)
        if (!near(view.m[i], default_view.m[i])) { differs = true; break; }
    CHECK(differs);
}

TEST_CASE("projection_matrix is non-degenerate") {
    Camera cam;
    math::Mat4 proj = cam.projection_matrix(16.0f / 9.0f);

    // Perspective matrix should have non-zero values
    bool has_nonzero = false;
    for (int i = 0; i < 16; ++i)
        if (!near(proj.m[i], 0.0f)) { has_nonzero = true; break; }
    CHECK(has_nonzero);
}

TEST_CASE("projection_matrix different aspect ratios") {
    Camera cam;

    math::Mat4 wide  = cam.projection_matrix(2.0f);
    math::Mat4 tall  = cam.projection_matrix(0.5f);

    // The projection matrices for different aspects should differ
    bool same = true;
    for (int i = 0; i < 16; ++i)
        if (!near(wide.m[i], tall.m[i])) { same = false; break; }
    CHECK(!same);  // should NOT be identical
}

TEST_CASE("Camera fov affects projection") {
    Camera wide_fov;
    wide_fov.fov_y_radians = 1.57f;  // 90 degrees

    Camera narrow_fov;
    narrow_fov.fov_y_radians = 0.52f;  // ~30 degrees

    math::Mat4 wide_proj   = wide_fov.projection_matrix(1.0f);
    math::Mat4 narrow_proj = narrow_fov.projection_matrix(1.0f);

    // Different FOVs should produce different matrices
    bool same = true;
    for (int i = 0; i < 16; ++i)
        if (!near(wide_proj.m[i], narrow_proj.m[i])) { same = false; break; }
    CHECK(!same);
}

} // TEST_SUITE("Camera")
