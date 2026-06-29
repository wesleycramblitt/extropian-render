#pragma once

#include <string>
#include <unordered_map>
#include <glad/gl.h>
#include <exd/render/texture.hpp>
#include <cstdint>

namespace exd::render {

class TextureManager {
public:
    /// Upload any ITextureSource to the GPU. Returns a handle.
    uint32_t upload(ITextureSource& source);

    /// Update an existing 3D texture (glTexSubImage3D).
    void update(uint32_t handle, ITextureSource& source);

    /// Bind to a texture unit. Returns the GPU texture.
    const TextureGPU* bind(uint32_t handle, GLenum texture_unit = GL_TEXTURE0);

    /// Delete the GL object.
    void destroy(uint32_t handle);

private:
    std::unordered_map<uint32_t, TextureGPU> textures_;
    uint32_t next_handle_ = 1;
};

} // namespace exd::render
