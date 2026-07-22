#include <exd/render/systems/primitive_mesh_system.hpp>
#include <exd/render/components/cube.hpp>
#include <exd/render/components/renderable.hpp>
#include <cstdio>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// PrimitiveMeshSystem
// ════════════════════════════════════════════════════════════════════

void PrimitiveMeshSystem::update_primitives(exd::ecs::Registry& registry) {
    for (auto e : registry.view<CubePrimitive>()) {
        auto& cube = registry.get<CubePrimitive>(e);
        Mesh mesh = create_cube_mesh(cube.size);
        uint32_t handle = ctx_.mesh_manager.create(mesh);
        if (registry.has<RenderableComponent>(e))
            registry.get<RenderableComponent>(e).mesh = handle;
        else {
            registry.emplace<RenderableComponent>(e, handle);
            std::printf("[PrimitiveMesh] Created cube size=%.1f for entity %u\n",
                        cube.size, e.id);
        }
    }
}

Mesh PrimitiveMeshSystem::create_cube_mesh(float size) {
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
        mesh.indices.insert(mesh.indices.end(), {start+0,start+1,start+2,start+0,start+2,start+3});
    }
    return mesh;
}

} // namespace exd::render
