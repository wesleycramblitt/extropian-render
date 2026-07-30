#include <exd/render/systems/grid_system.hpp>
#include <exd/render/components/grid.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/ecs/view.hpp>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// GridSystem
// ════════════════════════════════════════════════════════════════════

void GridSystem::update(exd::ecs::Registry& registry, double) {
    for (auto e : registry.view<GridComponent, Transform>()) {
        if (registry.has<Disabled>(e)) continue;
        auto& grid = registry.get<GridComponent>(e);
        if (window_->grid_visible && !registry.has<RenderableComponent>(e)) {
            Mesh mesh;
            mesh.topology = Topology::Lines;
            float s = grid.spacing > 0 ? grid.spacing : 1.0f;
            int N = 10; float extent = N * s;
            for (int i = -N; i <= N; ++i) {
                float c = i * s;
                math::Vec3f col{grid.color.w, grid.color.x, grid.color.y};
                Vertex v1; v1.position = {-extent, 0.0f, c}; v1.normal = col; mesh.vertices.push_back(v1);
                Vertex v2; v2.position = {+extent, 0.0f, c}; v2.normal = col; mesh.vertices.push_back(v2);
                Vertex v3; v3.position = {c, 0.0f, -extent}; v3.normal = col; mesh.vertices.push_back(v3);
                Vertex v4; v4.position = {c, 0.0f, +extent}; v4.normal = col; mesh.vertices.push_back(v4);
            }
            uint32_t handle = ctx_.mesh_manager.create(mesh);
            registry.emplace<RenderableComponent>(e, handle);
        } else if (!window_->grid_visible && registry.has<RenderableComponent>(e)) {
            registry.remove<RenderableComponent>(e);
        }
    }
}

} // namespace exd::render
