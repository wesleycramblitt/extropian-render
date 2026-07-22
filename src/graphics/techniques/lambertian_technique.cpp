#include <exd/render/graphics/techniques/lambertian_technique.hpp>
#include <exd/core/macros.hpp>

namespace exd::render {

void LambertianTechnique::bind(const math::Mat4& view, const math::Mat4& proj) {
    program_ = ctx_.shader_manager.get_or_load(
        "lambertian", "shaders/opengl/lambertian/lambertian.vert", "shaders/opengl/lambertian/lambertian.frag");
    GL_CALL(glUseProgram(program_));
    u_view_      = glGetUniformLocation(program_, "u_view");
    u_proj_      = glGetUniformLocation(program_, "u_proj");
    u_model_     = glGetUniformLocation(program_, "u_model");
    u_light_dir_ = glGetUniformLocation(program_, "u_light_dir");
    GL_CALL(glUniformMatrix4fv(u_view_, 1, GL_FALSE, view.m));
    GL_CALL(glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.m));
    GL_CALL(glUniform3f(u_light_dir_, 0.0f, -0.866f, -0.3f));
}

void LambertianTechnique::draw(uint32_t mesh_handle, const math::Mat4& model) {
    if (mesh_handle == 0) return;
    GL_CALL(glUniformMatrix4fv(u_model_, 1, GL_FALSE, model.m));
    const auto* mesh = ctx_.mesh_manager.bind(mesh_handle);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));
}

void LambertianTechnique::unbind() { GL_CALL(glUseProgram(0)); }

} // namespace exd::render
