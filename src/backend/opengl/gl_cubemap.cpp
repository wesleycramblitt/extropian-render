#include <exd/render/cubemap_texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/gl.h>
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
