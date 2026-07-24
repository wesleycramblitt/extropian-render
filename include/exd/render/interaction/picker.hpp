#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/render/graphics/mesh_manager.hpp>
#include <exd/render/interaction/ray.hpp>
#include <cstdint>
#include <optional>

namespace exd::render {

/// CPU-side raycasting picker.
/// Each frame, casts a ray from the camera through the mouse position
/// and finds the closest entity with a RenderableComponent.
class PickerSystem {
public:
    PickerSystem(MeshManager& mesh_mgr) : mesh_mgr_(mesh_mgr) {}

    /// Perform a pick at the given screen coordinates.
    /// `camera_pos` and `camera_forward` define the view ray origin.
    /// Returns the entity that was hit (closest to camera), or nullopt.
    std::optional<ecs::Entity> pick(
        ecs::Registry& registry,
        const math::Vec3f& camera_pos,
        const math::Vec3f& camera_forward,
        const math::Vec3f& camera_up,
        float fov_y_rad,
        float aspect,
        float screen_x, float screen_y,
        float screen_w, float screen_h) const;

private:
    MeshManager& mesh_mgr_;
};

} // namespace exd::render
