#include <exd/render/systems/mesh_asset_system.hpp>
#include <exd/render/components/mesh_asset.hpp>
#include <exd/render/components/renderable.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <functional>
#include <cstdio>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// MeshAssetSystem
// ════════════════════════════════════════════════════════════════════


void MeshAssetSystem::update_impl(exd::ecs::Registry& registry) {
    size_t count = 0;
    for (auto e : registry.view<MeshAssetComponent>()) {
        count++;
        auto& ma = registry.get<MeshAssetComponent>(e);
        std::printf("[MeshAsset] entity=%u path=%s\n", e.id, ma.path.c_str());
        if (ma.path.empty()) continue;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(ma.path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);
        if (!scene) {
            std::fprintf(stderr, "[MeshAsset] Assimp failed for %s: %s\n",
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
        std::printf("[MeshAsset] Loaded %s (%zu verts)\n",
                    ma.path.c_str(), mesh.vertices.size());
    }
}

} // namespace exd::render
