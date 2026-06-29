#include <exd/render/mesh_manager.hpp>
#include <stdexcept>

namespace exd::render {

uint32_t MeshManager::create(const Mesh& mesh) {
    uint32_t id = static_cast<uint32_t>(mesh_map_.size() + 1);
    mesh_map_.emplace(id, mesh);
    upload_to_gpu(mesh);
    return id;
}

uint32_t MeshManager::upload_to_gpu(const Mesh& mesh) {
    uint32_t id = static_cast<uint32_t>(meshgpu_map_.size() + 1);
    meshgpu_map_.try_emplace(id, mesh);
    return id;
}

const MeshGPU* MeshManager::bind(uint32_t mesh_handle) {
    auto it = meshgpu_map_.find(mesh_handle);
    if (it == meshgpu_map_.end())
        throw std::runtime_error("Mesh handle does not exist");
    it->second.bind();
    return &it->second;
}

const Mesh* MeshManager::get_mesh(uint32_t handle) const {
    auto it = mesh_map_.find(handle);
    return (it != mesh_map_.end()) ? &it->second : nullptr;
}

} // namespace exd::render
