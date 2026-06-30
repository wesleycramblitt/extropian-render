#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <exd/math/vec3.hpp>
#include <exd/math/quat.hpp>

namespace exd::render {

// ─── General-purpose ECS components for rendering ───

struct Transform {
    math::Vec3 position{0.0f, 0.0f, 0.0f};
    math::Quat rotation{1, 0, 0, 0};
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct Camera {
    float fov_y_radians = 1.047f;  // 60 degrees
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float exposure = 1.0f;
};

struct CameraController {
    float move_speed = 30.0f;
    float sprint_mult = 2.0f;
    float mouse_sensitivity = 0.002f;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct Skew {
    math::Vec3 shear{0.0f, 0.0f, 0.0f};  // xy, xz, yz shear factors
};

struct RenderableComponent {
    uint32_t mesh = 0;
};

struct CubeMapComponent {
    std::string name;
    bool cross_layout = true;
    uint32_t texture_handle = 0;
};

struct GridComponent {
    float spacing = 50.0f;
    math::Quat color{0.4f, 0.4f, 0.4f, 0.4f};
};

struct MeshAssetComponent {
    std::string path;
};

// Render technique tags (marker components)
struct RenderTechnique_Lambertian {};
struct RenderTechnique_Mirror {};
struct RenderTechnique_CubeMap {};
struct RenderTechnique_Lit {};

// Simulation components needed by the renderer
struct SimulationDomain {
    int nx = 64, ny = 64, nz = 64;
};

struct SimulationReference {
    uint32_t simulation_entity_id = 0;
};

struct ParticleCloudComponent {
    std::vector<float> positions;
    std::vector<float> colors;
    int particle_count = 0;
    int max_particles = 100000;
};

struct VolumeFieldComponent {
    uint32_t texture_handle = 0;
    bool interop_ready = false;
};

struct Disabled {};
struct Selected {};
struct ReadOnly {};

// Cube primitive
struct CubePrimitive {
    float size = 100.0f;
};

} // namespace exd::render
