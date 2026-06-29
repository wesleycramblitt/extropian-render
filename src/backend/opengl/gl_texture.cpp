#include <ext/render/texture_manager.hpp>
#include <ext/core/macros.hpp>
#include <stdexcept>

namespace ext::render {

uint32_t TextureManager::upload(ITextureSource& source) {
    GLuint id;
    glGenTextures(1, &id);
    GL_CALL(glBindTexture(source.gl_target(), id));

    int face_count = (source.gl_target() == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
    int mips = source.max_mip_levels();

    for (int face = 0; face < face_count; ++face)
        for (int level = 0; level < mips; ++level)
            source.upload_level(level, face);

    glTexParameteri(source.gl_target(), GL_TEXTURE_MIN_FILTER, source.min_filter());
    glTexParameteri(source.gl_target(), GL_TEXTURE_MAG_FILTER, source.mag_filter());
    glTexParameteri(source.gl_target(), GL_TEXTURE_WRAP_S, source.wrap_s());
    glTexParameteri(source.gl_target(), GL_TEXTURE_WRAP_T, source.wrap_t());
    if (source.gl_target() == GL_TEXTURE_3D || source.gl_target() == GL_TEXTURE_CUBE_MAP)
        glTexParameteri(source.gl_target(), GL_TEXTURE_WRAP_R, source.wrap_r());

    GL_CALL(glBindTexture(source.gl_target(), 0));

    uint32_t handle = next_handle_++;
    textures_.try_emplace(handle, id, source.gl_target());
    return handle;
}

void TextureManager::update(uint32_t handle, ITextureSource& source) {
    auto it = textures_.find(handle);
    if (it == textures_.end())
        throw std::runtime_error("TextureManager::update — handle not found");
    GL_CALL(glBindTexture(source.gl_target(), it->second.id));
    source.update_level(0, 0);
    GL_CALL(glBindTexture(source.gl_target(), 0));
}

const TextureGPU* TextureManager::bind(uint32_t handle, GLenum texture_unit) {
    auto it = textures_.find(handle);
    if (it == textures_.end())
        throw std::runtime_error("TextureManager::bind — handle not found");
    TextureGPU& gpu = it->second;
    GL_CALL(glActiveTexture(texture_unit));
    GL_CALL(glBindTexture(gpu.target, gpu.id));
    return &gpu;
}

void TextureManager::destroy(uint32_t handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        glDeleteTextures(1, &it->second.id);
        textures_.erase(it);
    }
}

} // namespace ext::render
