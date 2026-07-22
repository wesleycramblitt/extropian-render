#pragma once

#include <string>
#include <cstdint>
#include <glad/gl.h>

namespace exd::render {

/// CPU-side texture metadata.
struct Texture {
    std::string name;
    int width = 0;
    int height = 0;
    int depth = 1;
    int channels = 4;
    int mip_levels = 1;
};

/// GPU-side texture.
struct TextureGPU {
    GLuint id = 0;
    GLenum target = GL_TEXTURE_2D;
};

/// Abstract interface for uploading texture data to the GPU.
class ITextureSource {
public:
    virtual ~ITextureSource() = default;
    virtual GLenum gl_target() const = 0;
    virtual int max_mip_levels() const { return 1; }
    virtual void upload_level(int level, int face) = 0;
    virtual void update_level(int level, int face) { upload_level(level, face); }
    virtual GLint min_filter() const { return GL_LINEAR; }
    virtual GLint mag_filter() const { return GL_LINEAR; }
    virtual GLint wrap_s() const { return GL_CLAMP_TO_EDGE; }
    virtual GLint wrap_t() const { return GL_CLAMP_TO_EDGE; }
    virtual GLint wrap_r() const { return GL_CLAMP_TO_EDGE; }
};

} // namespace exd::render
