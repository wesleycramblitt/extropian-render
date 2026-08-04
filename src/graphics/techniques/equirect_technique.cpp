#include <exd/render/graphics/techniques/equirect_technique.hpp>
#include <exd/core/macros.hpp>

namespace exd::render {

void EquirectTechnique::bind(const math::Mat4& view, const math::Mat4& proj) {
    program_ = ctx_.shader_manager.get_or_load(
        "equirect", "shaders/opengl/equirect/equirect.vert",
        "shaders/opengl/equirect/equirect.frag");
    GL_CALL(glDepthFunc(GL_LEQUAL));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDisable(GL_CULL_FACE));
    GL_CALL(glUseProgram(program_));

    // Strip translation so the sphere appears infinitely distant
    math::Mat4 view_no_trans = view;
    view_no_trans.m[12] = 0.0f;
    view_no_trans.m[13] = 0.0f;
    view_no_trans.m[14] = 0.0f;

    math::Mat4 identity = math::Mat4::identity();
    GL_CALL(glUniformMatrix4fv(glGetUniformLocation(program_, "u_model"), 1, GL_FALSE, identity.m));
    GL_CALL(glUniformMatrix4fv(glGetUniformLocation(program_, "u_view"), 1, GL_FALSE, view_no_trans.m));
    GL_CALL(glUniformMatrix4fv(glGetUniformLocation(program_, "u_proj"), 1, GL_FALSE, proj.m));
}

void EquirectTechnique::draw(uint32_t mesh_handle, uint32_t texture_handle) {
    if (mesh_handle == 0 || texture_handle == 0) return;

    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_handle));
    GL_CALL(glUniform1i(glGetUniformLocation(program_, "u_equirect"), 0));

    const auto* mesh = ctx_.mesh_manager.bind(mesh_handle);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}

void EquirectTechnique::unbind() {
    GL_CALL(glDepthFunc(GL_LESS));
    GL_CALL(glDepthMask(GL_TRUE));
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glUseProgram(0));
}

} // namespace exd::render
