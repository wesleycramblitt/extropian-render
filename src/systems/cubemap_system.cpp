#include <exd/render/systems/cubemap_system.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/graphics/cubemap_texture.hpp>
#include <exd/ecs/view.hpp>
#include <exd/render/graphics/gl_loader.hpp>
#include <cstdio>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// CubeMapSystem
// ════════════════════════════════════════════════════════════════════

void CubeMapSystem::update_impl(exd::ecs::Registry& registry) {
    for (auto e : registry.view<CubeMapComponent>()) {
        auto& cm = registry.get<CubeMapComponent>(e);
        // Load texture if not already loaded
        if (cm.texture_handle == 0 && (!cm.name.empty() || !cm.custom_path.empty())) {
            std::string path = cm.custom_path.empty()
                ? "assets/cubemaps/" + cm.name + "/cross.png"
                : cm.custom_path;

            // Equirectangular (Radiance .hdr) panoramas -> float cube map
            auto is_hdr = [&](const std::string& p) {
                return p.size() > 4 &&
                       (p.substr(p.size() - 4) == ".hdr" || p.substr(p.size() - 4) == ".HDR");
            };
            if (is_hdr(path)) {
                CubeMapTexture tex = CubeMapTexture::from_equirectangular(path, 512);
                if (tex.valid()) {
                    tex.upload_to_gl_float();
                    cm.gl_cubemap = tex.release_gl();
                    cm.texture_handle = 1;
                    std::printf("[CubeMap] Uploaded equirect cubemap: %u\n", cm.gl_cubemap);
                }
            } else {
                CubeMapTexture tex(path, 512);
                if (tex.valid()) {
                    GLuint gl_tex;
                    glGenTextures(1, &gl_tex);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, gl_tex);
                    for (int i = 0; i < 6; ++i) {
                        auto& face = tex.get_face(i);
                        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8,
                                     face.width, face.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                     face.data.data());
                    }
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                    cm.gl_cubemap = gl_tex;
                    cm.texture_handle = 1;
                    std::printf("[CubeMap] Uploaded GL cubemap: %u\n", gl_tex);
                }
            }
        }
        // Create mesh if not already present
        if (!registry.has<RenderableComponent>(e)) {
            Mesh mesh = create_cubemap_mesh();
            uint32_t mesh_handle = ctx_.mesh_manager.create(mesh);
            registry.emplace<RenderableComponent>(e, mesh_handle);
        }
    }
}

Mesh CubeMapSystem::create_cubemap_mesh() {
    Mesh mesh;
    const float s = 100.0f;  // big enough to enclose any camera frustum (far plane is 1000)
    float v[] = {
        -s, s,-s,  -s,-s,-s,  s,-s,-s,  s,-s,-s,  s, s,-s, -s, s,-s,
        -s,-s, s,  -s,-s,-s,  -s, s,-s,  -s, s,-s,  -s, s, s, -s,-s, s,
         s,-s,-s,   s,-s, s,   s, s, s,   s, s, s,   s, s,-s,  s,-s,-s,
        -s,-s, s,  -s, s, s,   s, s, s,   s, s, s,   s,-s, s, -s,-s, s,
        -s, s,-s,   s, s,-s,   s, s, s,   s, s, s,  -s, s, s, -s, s,-s,
        -s,-s,-s,  -s,-s, s,   s,-s,-s,   s,-s,-s,  -s,-s, s,  s,-s, s,
    };
    for (size_t i = 0; i < 108; i += 3)
        mesh.vertices.push_back({{v[i], v[i+1], v[i+2]}});
    return mesh;
}

} // namespace exd::render
