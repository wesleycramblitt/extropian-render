#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace exd::render {

/// Simple cubemap texture loader using stb_image.
/// Supports cross-layout (single PNG with 6 faces in a cross pattern),
/// individual face files, and equirectangular (Radiance .hdr) panoramas.
class CubeMapTexture {
public:
    /// Load from a cross-layout PNG (like the "10" cubemap).
    /// face_size is the size of each square face in the cross.
    CubeMapTexture(const std::string& cross_path, int face_size_hint = 512);

    CubeMapTexture() = default;

    /// Load from 6 individual face files.
    CubeMapTexture(const std::string* face_paths);

    /// Load from an equirectangular (2:1) panorama, e.g. Radiance .hdr.
    /// Converts to a cube map of face_size×face_size per face.
    static CubeMapTexture from_equirectangular(const std::string& path, int face_size = 512);

    ~CubeMapTexture();

    // GPU upload helpers
    struct Face {
        int width, height, channels;
        std::vector<uint8_t> data;   // 8-bit RGBA
        std::vector<float> data_f;   // float RGB (equirectangular HDR path)
    };

    [[nodiscard]] const Face& get_face(int index) const { return faces_[index]; }
    [[nodiscard]] int face_size() const { return faces_[0].width; }
    [[nodiscard]] bool has_float() const { return !faces_.empty() && !faces_[0].data_f.empty(); }

    /// Upload to OpenGL (called by TextureManager).
    void upload_to_gl() const;

    /// Upload float (HDR) faces as GL_RGB16F.
    void upload_to_gl_float() const;

    /// Check if loading succeeded
    [[nodiscard]] bool valid() const {
        return !faces_.empty() && (!faces_[0].data.empty() || !faces_[0].data_f.empty());
    }

    [[nodiscard]] uint32_t gl_handle() const { return gl_handle_; }

    /// Transfer ownership of the uploaded GL texture (stops the destructor from deleting it).
    [[nodiscard]] uint32_t release_gl() {
        uint32_t h = gl_handle_;
        gl_handle_ = 0;
        return h;
    }

private:
    void build_faces_from_equirect(const float* img, int w, int h, int face_size);
    std::vector<Face> faces_;
    uint32_t gl_handle_ = 0;
};

} // namespace exd::render
