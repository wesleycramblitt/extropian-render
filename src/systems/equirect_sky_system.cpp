#include <exd/render/systems/equirect_sky_system.hpp>
#include <exd/render/components/equirect_sky.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/ecs/view.hpp>
#include <exd/render/graphics/mesh.hpp>
#include <exd/render/graphics/gl_loader.hpp>
#include <stb_image.h>

#include <cmath>
#include <cstdio>
#include <vector>

namespace exd::render {

static Mesh generate_sphere(uint32_t lon_segments, uint32_t lat_segments, float radius) {
    Mesh mesh;
    mesh.topology = Topology::Triangles;

    // Generate vertices
    for (uint32_t lat = 0; lat <= lat_segments; ++lat) {
        float theta = static_cast<float>(lat) / lat_segments * 3.141592653589793f;  // 0 to PI
        float sin_theta = std::sin(theta);
        float cos_theta = std::cos(theta);
        for (uint32_t lon = 0; lon <= lon_segments; ++lon) {
            float phi = static_cast<float>(lon) / lon_segments * 2.0f * 3.141592653589793f;
            float sin_phi = std::sin(phi);
            float cos_phi = std::cos(phi);
            Vertex v;
            v.position = {radius * sin_theta * cos_phi, radius * cos_theta, radius * sin_theta * sin_phi};
            mesh.vertices.push_back(v);
        }
    }

    // Generate indices (two triangles per quad)
    uint32_t vps = lon_segments + 1;  // vertices per strip
    for (uint32_t lat = 0; lat < lat_segments; ++lat) {
        for (uint32_t lon = 0; lon < lon_segments; ++lon) {
            uint32_t a = lat * vps + lon;
            uint32_t b = a + 1;
            uint32_t c = (lat + 1) * vps + lon;
            uint32_t d = c + 1;
            if (lat != 0) {
                mesh.indices.push_back(a);
                mesh.indices.push_back(c);
                mesh.indices.push_back(b);
            }
            if (lat != lat_segments - 1) {
                mesh.indices.push_back(b);
                mesh.indices.push_back(c);
                mesh.indices.push_back(d);
            }
        }
    }

    return mesh;
}

void EquirectSkySystem::update(exd::ecs::Registry& registry, double /*dt*/) {
    // Ensure the shared sphere mesh exists
    if (sphere_mesh_ == 0) {
        Mesh sphere = generate_sphere(64, 32, 100.0f);
        sphere_mesh_ = ctx_.mesh_manager.create(sphere);
    }

    for (auto e : registry.view<EquirectSkyComponent>()) {
        auto& esc = registry.get<EquirectSkyComponent>(e);
        if (esc.path.empty()) continue;

        // Load the HDR texture if not already loaded
        if (esc.gl_texture == 0) {
            int w, h, ch;
            float* img = stbi_loadf(esc.path.c_str(), &w, &h, &ch, 3);
            if (!img) {
                std::fprintf(stderr, "[EquirectSky] Failed to load: %s — %s\n",
                             esc.path.c_str(), stbi_failure_reason());
                continue;
            }
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, img);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            stbi_image_free(img);
            esc.gl_texture = tex;
            std::printf("[EquirectSky] Loaded %s (%dx%d) as GL 2D texture %u\n",
                        esc.path.c_str(), w, h, tex);
        }

        // Assign the shared sphere mesh if not already done
        if (!registry.has<RenderableComponent>(e)) {
            registry.emplace<RenderableComponent>(e, sphere_mesh_);
        }
    }
}

} // namespace exd::render
