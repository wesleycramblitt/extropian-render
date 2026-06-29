#pragma once

#include <exd/render/mesh.hpp>
#include <exd/render/mesh_gpu.hpp>
#include <unordered_map>
#include <cstdint>

namespace exd::render {

class MeshManager {
public:
    uint32_t create(const Mesh& mesh);
    const MeshGPU* bind(uint32_t mesh_handle);
    const Mesh* get_mesh(uint32_t handle) const;

private:
    uint32_t upload_to_gpu(const Mesh& mesh);
    std::unordered_map<uint32_t, Mesh> mesh_map_;
    std::unordered_map<uint32_t, MeshGPU> meshgpu_map_;
};

} // namespace exd::render
