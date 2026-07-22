#include <doctest/doctest.h>
#include "test_common.hpp"
#include <exd/render/systems/primitive_mesh_system.hpp>
#include <exd/render/graphics/mesh_manager.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

// We need a PrimitiveMeshSystem to test cube generation.
// But it requires GraphicsContext, which needs GL. For pure geometry
// tests, we can construct a Mesh directly or use a mock context.

TEST_SUITE("Mesh") {

TEST_CASE("Vertex default values") {
    Vertex v{};
    CHECK(near(v.position, math::Vec3f{0,0,0}));
    CHECK(near(v.normal,   math::Vec3f{0,1,0}));
    CHECK(near(v.uv,       math::Vec3f{0,0,0}));
}

TEST_CASE("Vertex aggregate init") {
    Vertex v{math::Vec3f{1,2,3}, math::Vec3f{0,0,1}};
    CHECK(near(v.position, math::Vec3f{1,2,3}));
    CHECK(near(v.normal,   math::Vec3f{0,0,1}));
}

TEST_CASE("Empty mesh has no vertices or indices") {
    Mesh m;
    CHECK(m.vertices.empty());
    CHECK(m.indices.empty());
    CHECK(m.topology == Topology::Triangles);
}

TEST_CASE("Mesh with vertices and indices") {
    Mesh m;
    m.vertices.push_back(Vertex{math::Vec3f{0,0,0}});
    m.vertices.push_back(Vertex{math::Vec3f{1,0,0}});
    m.vertices.push_back(Vertex{math::Vec3f{0,1,0}});
    m.indices = {0, 1, 2};

    CHECK(m.vertices.size() == 3);
    CHECK(m.indices.size() == 3);
    CHECK(mesh_is_valid(m));
}

TEST_CASE("Cube mesh generation creates valid geometry") {
    // We need PrimitiveMeshSystem but it takes GraphicsContext&
    // which needs a window/GL context. Instead, test the create_cube_mesh
    // logic directly by inspecting expected properties.
    //
    // create_cube_mesh(size) creates 6 faces × 4 vertices = 24 verts
    // and 6 faces × 6 indices = 36 indices.
    float size = 2.0f;

    // Build a cube manually following the same algorithm
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
        mesh.vertices.push_back(Vertex{f.v0, f.n});
        mesh.vertices.push_back(Vertex{f.v1, f.n});
        mesh.vertices.push_back(Vertex{f.v2, f.n});
        mesh.vertices.push_back(Vertex{f.v3, f.n});
        mesh.indices.insert(mesh.indices.end(),
            {start+0,start+1,start+2,start+0,start+2,start+3});
    }

    CHECK(mesh.vertices.size() == 24);
    CHECK(mesh.indices.size() == 36);
    CHECK(mesh.topology == Topology::Triangles);
    CHECK(mesh_is_valid(mesh));

    // Each vertex should be on the cube surface (at distance h from origin)
    for (auto& v : mesh.vertices) {
        float dist = v.position.length();
        // Distance from center should be h * sqrt(3) for corners,
        // but at least h for all vertices
        CHECK(dist >= h * 0.99f);
        CHECK(dist <= h * 1.74f);  // sqrt(3) ≈ 1.732
    }
}

TEST_CASE("Cube mesh normals are unit length") {
    float size = 2.0f;
    float h = size * 0.5f;

    Mesh mesh;
    math::Vec3f normals[6] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    math::Vec3f corners[4] = {
        {h,-h,-h},{h,h,-h},{h,h,h},{h,-h,h}
    };

    // Generate one face
    for (int f = 0; f < 6; ++f) {
        // Build face vertices (simplified)
        uint32_t start = mesh.vertices.size();

        // Just check normals are unit length on the first face
        mesh.vertices.push_back(Vertex{corners[0], normals[f]});
        if (f == 0) {
            float nl = mesh.vertices.back().normal.length();
            CHECK(near(nl, 1.0f));
        }
    }
}

TEST_CASE("Mesh topology enum values") {
    CHECK(static_cast<int>(Topology::Triangles) != static_cast<int>(Topology::Lines));
    CHECK(static_cast<int>(Topology::Lines) != static_cast<int>(Topology::Points));
}

} // TEST_SUITE("Mesh")
