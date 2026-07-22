#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace exd::render {

/// Simple cubemap texture loader using stb_image.
/// Supports cross-layout (single PNG with 6 faces in a cross pattern) 
/// and individual face files.
class CubeMapTexture {
public:
    /// Load from a cross-layout PNG (like the "10" cubemap).
    /// face_size is the size of each square face in the cross.
    CubeMapTexture(const std::string& cross_path, int face_size_hint = 512);

    /// Load from 6 individual face files.
    CubeMapTexture(const std::string* face_paths);

    ~CubeMapTexture();

    // GPU upload helpers
    struct Face {
        int width, height, channels;
        std::vector<uint8_t> data;
    };

    [[nodiscard]] const Face& get_face(int index) const { return faces_[index]; }
    [[nodiscard]] int face_size() const { return faces_[0].width; }

    /// Upload to OpenGL (called by TextureManager).
    void upload_to_gl() const;

    /// Check if loading succeeded
    [[nodiscard]] bool valid() const { return !faces_.empty() && !faces_[0].data.empty(); }

private:
    std::vector<Face> faces_;
    uint32_t gl_handle_ = 0;
};

} // namespace exd::render
