#include <exd/render/systems/mesh_asset_system.hpp>
#include <exd/render/components/mesh_asset.hpp>
#include <exd/render/components/renderable.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <functional>
#include <unordered_map>

namespace exd::render {

void MeshAssetSystem::update_impl(exd::ecs::Registry& registry) {
    // Path → mesh handle cache so each file is loaded only once
    std::unordered_map<std::string, uint32_t> loaded;

    for (auto e : registry.view<MeshAssetComponent>()) {
        auto& ma = registry.get<MeshAssetComponent>(e);
        if (ma.path.empty()) continue;

        // Skip if already assigned a valid mesh
        if (registry.has<RenderableComponent>(e) &&
            registry.get<RenderableComponent>(e).mesh != 0)
            continue;

        // Check cache
        auto it = loaded.find(ma.path);
        if (it != loaded.end()) {
            registry.emplace<RenderableComponent>(e, it->second);
            continue;
        }

        // Load via Assimp
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(ma.path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);
        if (!scene) {
            std::fprintf(stderr, "[MeshAsset] Failed: %s — %s\n",
                         ma.path.c_str(), importer.GetErrorString());
            continue;
        }

        Mesh mesh;
        mesh.topology = Topology::Triangles;

        std::function<void(const aiNode*)> process = [&](const aiNode* node) {
            for (unsigned i = 0; i < node->mNumMeshes; ++i) {
                const aiMesh* m = scene->mMeshes[node->mMeshes[i]];
                uint32_t base = mesh.vertices.size();
                for (unsigned v = 0; v < m->mNumVertices; ++v) {
                    Vertex vert;
                    vert.position = {m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z};
                    if (m->HasNormals())
                        vert.normal = {m->mNormals[v].x, m->mNormals[v].y, m->mNormals[v].z};
                    mesh.vertices.push_back(vert);
                }
                for (unsigned f = 0; f < m->mNumFaces; ++f)
                    for (unsigned k = 0; k < m->mFaces[f].mNumIndices; ++k)
                        mesh.indices.push_back(base + m->mFaces[f].mIndices[k]);
            }
            for (unsigned c = 0; c < node->mNumChildren; ++c)
                process(node->mChildren[c]);
        };
        process(scene->mRootNode);

        uint32_t handle = ctx_.mesh_manager.create(mesh);
        registry.emplace<RenderableComponent>(e, handle);
        loaded[ma.path] = handle;
    }
}

} // namespace exd::render
