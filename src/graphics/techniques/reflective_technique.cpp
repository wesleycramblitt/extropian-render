#include <exd/render/graphics/techniques/reflective_technique.hpp>
#include <exd/core/macros.hpp>

namespace exd::render {

void ReflectiveTechnique::bind(const math::Mat4& view, const math::Mat4& proj,
                                const math::Vec3f& cam_pos, uint32_t cubemap_handle) {
    if (cubemap_handle == 0) return;
    program_ = ctx_.shader_manager.get_or_load(
        "reflective", "shaders/opengl/reflective/reflective.vert", "shaders/opengl/reflective/reflective.frag");
    GL_CALL(glUseProgram(program_));
    u_view_    = glGetUniformLocation(program_, "u_view");
    u_proj_    = glGetUniformLocation(program_, "u_proj");
    u_model_   = glGetUniformLocation(program_, "u_model");
    u_cam_pos_ = glGetUniformLocation(program_, "u_camPos");
    u_skybox_  = glGetUniformLocation(program_, "u_skybox");
    GL_CALL(glUniformMatrix4fv(u_view_, 1, GL_FALSE, view.m));
    GL_CALL(glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.m));
    GL_CALL(glUniform3f(u_cam_pos_, cam_pos.x, cam_pos.y, cam_pos.z));
    // Bind cubemap directly (raw GL handle)
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_handle));
    GL_CALL(glUniform1i(u_skybox_, 0));
}

void ReflectiveTechnique::draw(uint32_t mesh_handle, const math::Mat4& model) {
    if (mesh_handle == 0) return;
    GL_CALL(glUniformMatrix4fv(u_model_, 1, GL_FALSE, model.m));
    const auto* mesh = ctx_.mesh_manager.bind(mesh_handle);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));
}

void ReflectiveTechnique::unbind() {
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
    GL_CALL(glUseProgram(0));
}

} // namespace exd::render
