#include <ext/render/techniques.hpp>
#include <ext/core/macros.hpp>

namespace ext::render {

void LambertianTechnique::bind(const math::Mat4& view, const math::Mat4& proj) {
    program_ = ctx_.shader_manager.get_or_load(
        "lambertian", "shaders/lambertian/lambertian.vert", "shaders/lambertian/lambertian.frag");
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

// ── Reflective ──────────────────────────────────────────

void ReflectiveTechnique::bind(const math::Mat4& view, const math::Mat4& proj,
                                const math::Vec3& cam_pos, uint32_t cubemap_handle) {
    if (cubemap_handle == 0) return;
    program_ = ctx_.shader_manager.get_or_load(
        "reflective", "shaders/reflective/reflective.vert", "shaders/reflective/reflective.frag");
    GL_CALL(glUseProgram(program_));
    u_view_    = glGetUniformLocation(program_, "u_view");
    u_proj_    = glGetUniformLocation(program_, "u_proj");
    u_model_   = glGetUniformLocation(program_, "u_model");
    u_cam_pos_ = glGetUniformLocation(program_, "u_camPos");
    u_skybox_  = glGetUniformLocation(program_, "u_skybox");
    GL_CALL(glUniformMatrix4fv(u_view_, 1, GL_FALSE, view.m));
    GL_CALL(glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.m));
    GL_CALL(glUniform3f(u_cam_pos_, cam_pos.x, cam_pos.y, cam_pos.z));
    ctx_.texture_manager.bind(cubemap_handle);
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

// ── CubeMap ──────────────────────────────────────────────

void CubeMapRenderTechnique::bind() {
    cubemap_program_ = ctx_.shader_manager.get_or_load(
        "cubemap", "shaders/cubemap/cubemap.vert", "shaders/cubemap/cubemap.frag");
    GL_CALL(glDepthFunc(GL_LEQUAL));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDisable(GL_CULL_FACE));
    GL_CALL(glUseProgram(cubemap_program_));
}

void CubeMapRenderTechnique::draw(const Renderable& renderable) {
    GLint u_skybox = glGetUniformLocation(cubemap_program_, "u_skybox");
    GLint u_view = glGetUniformLocation(cubemap_program_, "u_view");
    GLint u_proj = glGetUniformLocation(cubemap_program_, "u_proj");

    GL_CALL(glUniformMatrix4fv(u_view, 1, GL_FALSE, std::get<math::Mat4>(renderable.uniforms.at("u_view")).m));
    GL_CALL(glUniformMatrix4fv(u_proj, 1, GL_FALSE, std::get<math::Mat4>(renderable.uniforms.at("u_proj")).m));

    if (renderable.mesh_handle == 0 || renderable.texture_handle == 0) return;

    ctx_.texture_manager.bind(renderable.texture_handle);
    GL_CALL(glUniform1i(u_skybox, 0));

    const auto* mesh = ctx_.mesh_manager.bind(renderable.mesh_handle);
    GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

void CubeMapRenderTechnique::unbind() {
    GL_CALL(glDepthFunc(GL_LESS));
    GL_CALL(glDepthMask(GL_TRUE));
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glUseProgram(0));
}

} // namespace ext::render
