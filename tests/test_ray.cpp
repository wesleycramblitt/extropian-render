#include <doctest/doctest.h>
#include "test_common.hpp"
#include <exd/render/interaction/ray.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;
namespace intr = exd::render::interaction;

TEST_SUITE("Ray math") {

TEST_CASE("screen_ray through center of viewport") {
    // Camera at origin, looking down -Z
    auto ray = intr::screen_ray(
        {0,0,0}, {0,0,-1}, {0,1,0},  // eye, forward, up
        1.047f, 1.0f,                 // fov_y_rad, aspect
        400.0f, 300.0f, 800.0f, 600.0f  // screen_x, screen_y, screen_w, screen_h
    );
    CHECK(near(ray.origin, math::Vec3f{0,0,0}));
    // Center of screen should be along forward direction
    CHECK(near(ray.direction, math::Vec3f{0,0,-1}));
}

TEST_CASE("screen_ray top-left corner aims up-left") {
    auto ray = intr::screen_ray(
        {0,0,0}, {0,0,-1}, {0,1,0},
        1.047f, 16.0f/9.0f,
        0.0f, 0.0f, 800.0f, 600.0f
    );
    // Top-left = negative X, positive Y (Y is flipped: screen 0 = top = +Y in NDC)
    CHECK(ray.direction.x < -0.3f);  // left
    CHECK(ray.direction.y > 0.2f);   // up
    CHECK(ray.direction.z < 0.0f);   // forward direction is negative Z
}

TEST_CASE("ray_triangle hits front-facing triangle") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    math::Vec3f v0{ -1, -1, 0 };
    math::Vec3f v1{  1, -1, 0 };
    math::Vec3f v2{  0,  1, 0 };

    auto t = intr::ray_triangle(ray, v0, v1, v2);
    CHECK(t.has_value());
    CHECK(near(*t, 5.0f));  // triangle at z=0, ray from z=5
}

TEST_CASE("ray_triangle misses when ray is parallel") {
    intr::Ray ray{{0,0,5}, {1,0,0}};
    math::Vec3f v0{ -1, -1, 0 };
    math::Vec3f v1{  1, -1, 0 };
    math::Vec3f v2{  0,  1, 0 };

    auto t = intr::ray_triangle(ray, v0, v1, v2);
    CHECK(!t.has_value());
}

TEST_CASE("ray_triangle hits from both sides") {
    // Möller-Trumbore does NOT backface-cull — it returns hits from
    // both sides of the triangle.
    intr::Ray ray_from_front{{0,0,5}, {0,0,-1}};
    intr::Ray ray_from_back {{0,0,-5},{0,0,1}};
    math::Vec3f v0{ -1, -1, 0 };
    math::Vec3f v1{  1, -1, 0 };
    math::Vec3f v2{  0,  1, 0 };

    auto t_front = intr::ray_triangle(ray_from_front, v0, v1, v2);
    auto t_back  = intr::ray_triangle(ray_from_back, v0, v1, v2);

    CHECK(t_front.has_value());
    CHECK(t_back.has_value());
    CHECK(near(*t_front, 5.0f));
    CHECK(near(*t_back,  5.0f));
}

TEST_CASE("ray_plane intersection perpendicular") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    auto t = intr::ray_plane(ray, {0,0,0}, {0,0,1});
    CHECK(t.has_value());
    CHECK(near(*t, 5.0f));
}

TEST_CASE("ray_plane parallel returns nullopt") {
    intr::Ray ray{{0,0,5}, {1,0,0}};
    auto t = intr::ray_plane(ray, {0,0,0}, {0,0,1});
    CHECK(!t.has_value());
}

TEST_CASE("ray_sphere hits center") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    auto t = intr::ray_sphere(ray, {0,0,0}, 1.0f);
    CHECK(t.has_value());
    CHECK(near(*t, 4.0f));  // sphere center at 0, ray at 5, radius 1 → hit at t=4
}

TEST_CASE("ray_sphere misses") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    auto t = intr::ray_sphere(ray, {10,0,0}, 1.0f);
    CHECK(!t.has_value());
}

TEST_CASE("ray_sphere ray origin inside sphere") {
    intr::Ray ray{{0,0,0}, {0,0,-1}};
    auto t = intr::ray_sphere(ray, {0,0,0}, 3.0f);
    CHECK(t.has_value());
    // Should return exit point t (positive)
    CHECK(*t > 0.0f);
}

TEST_CASE("ray_aabb hits centered box") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    auto t = intr::ray_aabb(ray, {-1,-1,-1}, {1,1,1});
    CHECK(t.has_value());
}

TEST_CASE("ray_aabb misses box to the side") {
    intr::Ray ray{{0,0,5}, {0,0,-1}};
    auto t = intr::ray_aabb(ray, {10,10,10}, {11,11,11});
    CHECK(!t.has_value());
}

TEST_CASE("ray_aabb ray origin inside box") {
    intr::Ray ray{{0,0,0}, {0,0,-1}};
    auto t = intr::ray_aabb(ray, {-2,-2,-2}, {2,2,2});
    CHECK(t.has_value());
}

TEST_CASE("closest_point_on_ray projects point onto ray") {
    math::Vec3f origin{0,0,0};
    math::Vec3f dir{1,0,0};
    math::Vec3f point{5, 3, 0};

    math::Vec3f closest = intr::closest_point_on_ray(origin, dir, point);
    CHECK(near(closest, math::Vec3f{5,0,0}));
}

} // TEST_SUITE("Ray math")
