#include <exd/render/graphics/cubemap_texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <exd/render/graphics/gl_loader.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace exd::render {

CubeMapTexture::CubeMapTexture(const std::string& cross_path, int face_size_hint) {
    // Load cross-layout PNG
    int w, h, ch;
    uint8_t* img = stbi_load(cross_path.c_str(), &w, &h, &ch, 4); // force RGBA
    if (!img) {
        std::fprintf(stderr, "[CubeMap] Failed to load: %s\n", cross_path.c_str());
        return;
    }

    // Cross layout: 3x4 grid of square faces
    // Layout (row, col): 
    //   row 0: empty, +Y, empty, empty
    //   row 1: -X, +Z, +X, -Z  
    //   row 2: empty, -Y, empty, empty
    int face_w = face_size_hint;
    if (face_w <= 0) face_w = w / 4;  // guess from width
    
    // Clamp to actual image size (cross layout: 4 faces wide, 3 tall)
    int max_face_w = w / 4;
    int max_face_h = h / 3;
    face_w = std::min({face_w, max_face_w, max_face_h});
    if (face_w <= 0) {
        std::fprintf(stderr, "[CubeMap] Invalid cross-layout dimensions: %dx%d\n", w, h);
        stbi_image_free(img);
        return;
    }

    // Map cross grid positions to cubemap face indices (+X, -X, +Y, -Y, +Z, -Z)
    struct { int col, row, face; } mappings[] = {
        {2, 1, 0}, // +X (right)
        {0, 1, 1}, // -X (left)
        {1, 0, 2}, // +Y (top)
        {1, 2, 3}, // -Y (bottom)
        {1, 1, 4}, // +Z (front)
        {3, 1, 5}, // -Z (back)
    };

    faces_.resize(6);
    for (auto& m : mappings) {
        int src_x = m.col * face_w;
        int src_y = m.row * face_w;
        Face& face = faces_[m.face];
        face.width = face_w;
        face.height = face_w;
        face.channels = 4;
        face.data.resize(face_w * face_w * 4);

        // Copy pixels
        for (int y = 0; y < face_w; ++y) {
            for (int x = 0; x < face_w; ++x) {
                int src_idx = ((src_y + y) * w + (src_x + x)) * 4;
                int dst_idx = (y * face_w + x) * 4;
                for (int c = 0; c < 4; ++c)
                    face.data[dst_idx + c] = img[src_idx + c];
            }
        }
    }

    stbi_image_free(img);
    std::printf("[CubeMap] Loaded cross cubemap: %dx%d, face=%d\n", w, h, face_w);
}

CubeMapTexture::CubeMapTexture(const std::string* face_paths) {
    faces_.resize(6);
    for (int i = 0; i < 6; ++i) {
        int w, h, ch;
        uint8_t* img = stbi_load(face_paths[i].c_str(), &w, &h, &ch, 4);
        if (!img) {
            std::fprintf(stderr, "[CubeMap] Failed face %d: %s\n", i, face_paths[i].c_str());
            faces_.clear();
            return;
        }
        Face& face = faces_[i];
        face.width = w; face.height = h; face.channels = 4;
        face.data.assign(img, img + w * h * 4);
        stbi_image_free(img);
    }
}

CubeMapTexture::~CubeMapTexture() {
    if (gl_handle_) glDeleteTextures(1, &gl_handle_);
}

// ── Equirectangular (.hdr) support ──────────────────────────────────────

CubeMapTexture CubeMapTexture::from_equirectangular(const std::string& path, int face_size) {
    CubeMapTexture tex;
    int w, h, ch;
    float* img = stbi_loadf(path.c_str(), &w, &h, &ch, 3);  // float RGB
    if (!img) {
        std::fprintf(stderr, "[CubeMap] Failed to load equirect: %s\n", path.c_str());
        return tex;
    }
    if (w < 2 || h < 2 || w != h * 2) {
        std::fprintf(stderr, "[CubeMap] Not a 2:1 equirect: %s (%dx%d)\n", path.c_str(), w, h);
        stbi_image_free(img);
        return tex;
    }
    tex.build_faces_from_equirect(img, w, h, face_size);
    stbi_image_free(img);
    std::printf("[CubeMap] Loaded equirect %s: %dx%d -> faces %d\n",
                path.c_str(), w, h, face_size);
    return tex;
}

void CubeMapTexture::build_faces_from_equirect(const float* img, int w, int h, int face_size) {
    faces_.resize(6);
    // GL cube map direction conventions (s,t in [0,1], t=0 = first data row):
    //   +X:(1,-ty,-sx) -X:(-1,-ty,sx) +Y:(sx,1,ty) -Y:(sx,-1,-ty)
    //   +Z:(-sx,-ty,1) -Z:(sx,-ty,-1)   where sx=2s-1, ty=2t-1
    for (int face = 0; face < 6; ++face) {
        Face& f = faces_[face];
        f.width = f.height = face_size;
        f.channels = 3;
        f.data_f.resize(static_cast<size_t>(face_size) * face_size * 3);
        for (int ty = 0; ty < face_size; ++ty) {
            double t = (ty + 0.5) / face_size;
            double tyv = 2.0 * t - 1.0;
            for (int sx = 0; sx < face_size; ++sx) {
                double s = (sx + 0.5) / face_size;
                double sxv = 2.0 * s - 1.0;
                double dx, dy, dz;
                switch (face) {
                    case 0: dx = 1.0;  dy = -tyv; dz = -sxv; break;  // +X
                    case 1: dx = -1.0; dy = -tyv; dz =  sxv; break;  // -X
                    case 2: dx = sxv;  dy = 1.0;  dz =  tyv; break;  // +Y
                    case 3: dx = sxv;  dy = -1.0; dz = -tyv; break;  // -Y
                    case 4: dx = -sxv; dy = -tyv; dz = 1.0;  break;  // +Z
                    default: dx = sxv; dy = -tyv; dz = -1.0; break;  // -Z
                }
                double inv = 1.0 / std::sqrt(dx * dx + dy * dy + dz * dz);
                dx *= inv; dy *= inv; dz *= inv;
                // equirectangular: u wraps horizontally, v clamps at poles
                double u = 0.5 + std::atan2(dx, dz) / (2.0 * M_PI);
                double v = 0.5 - std::asin(dy) / M_PI;
                double fu = u * w - 0.5;
                double fv = v * h - 0.5;
                int x0 = (int)std::floor(fu);
                int y0 = (int)std::floor(fv);
                double fx_ = fu - x0, fy_ = fv - y0;
                auto wrap = [&](int x) { x %= w; return x < 0 ? x + w : x; };
                auto clampv = [&](int y) { return y < 0 ? 0 : (y > h - 1 ? h - 1 : y); };
                int x1 = wrap(x0 + 1);
                int y1 = clampv(y0 + 1);
                y0 = clampv(y0);
                for (int c = 0; c < 3; ++c) {
                    double p00 = img[(size_t)y0 * w * 3 + (size_t)wrap(x0) * 3 + c];
                    double p10 = img[(size_t)y0 * w * 3 + (size_t)x1 * 3 + c];
                    double p01 = img[(size_t)y1 * w * 3 + (size_t)wrap(x0) * 3 + c];
                    double p11 = img[(size_t)y1 * w * 3 + (size_t)x1 * 3 + c];
                    double top = p00 * (1.0 - fx_) + p10 * fx_;
                    double bot = p01 * (1.0 - fx_) + p11 * fx_;
                    double val = top * (1.0 - fy_) + bot * fy_;
                    f.data_f[((size_t)ty * face_size + sx) * 3 + c] = (float)val;
                }
            }
        }
    }
}

void CubeMapTexture::upload_to_gl_float() const {
    if (!valid() || !has_float()) return;
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    for (int i = 0; i < 6; ++i) {
        const auto& face = faces_[i];
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     face.width, face.height, 0, GL_RGB, GL_FLOAT,
                     face.data_f.data());
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    *(const_cast<uint32_t*>(&gl_handle_)) = tex;
}

void CubeMapTexture::upload_to_gl() const {
    if (!valid()) return;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    for (int i = 0; i < 6; ++i) {
        const auto& face = faces_[i];
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8,
                     face.width, face.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     face.data.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    *(const_cast<uint32_t*>(&gl_handle_)) = tex;
}

} // namespace exd::render
