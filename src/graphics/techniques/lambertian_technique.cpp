#include <exd/render/graphics/techniques/lambertian_technique.hpp>
#include <exd/core/macros.hpp>

namespace exd::render {

void LambertianTechnique::bind(const math::Mat4& view, const math::Mat4& proj,
                                const math::Vec3f& cam_pos,
                                const math::Vec3f& fog_color, float fog_density,
                                const math::Vec3f& ambient,
                                const math::Vec3f& sun_dir,
                                const math::Vec3f& sun_color) {
    program_ = ctx_.shader_manager.get_or_load(
        "lambertian", "shaders/opengl/lambertian/lambertian.vert",
        "shaders/opengl/lambertian/lambertian.frag");
    GL_CALL(glUseProgram(program_));

    // Disable face culling — the cubemap/equirect passes above us may
    // have re-enabled it. UI panels (extrusion caps) need both sides visible.
    GL_CALL(glDisable(GL_CULL_FACE));

    u_view_       = glGetUniformLocation(program_, "u_view");
    u_proj_       = glGetUniformLocation(program_, "u_proj");
    u_model_      = glGetUniformLocation(program_, "u_model");
    u_light_dir_  = glGetUniformLocation(program_, "u_light_dir");
    u_baseColor_  = glGetUniformLocation(program_, "u_baseColor");
    u_ambient_    = glGetUniformLocation(program_, "u_ambient");
    u_sun_color_  = glGetUniformLocation(program_, "u_sun_color");
    u_fog_color_  = glGetUniformLocation(program_, "u_fog_color");
    u_fog_density_= glGetUniformLocation(program_, "u_fog_density");
    u_cam_pos_    = glGetUniformLocation(program_, "u_cam_pos");

    GL_CALL(glUniformMatrix4fv(u_view_, 1, GL_FALSE, view.m));
    GL_CALL(glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.m));
    GL_CALL(glUniform3f(u_cam_pos_, cam_pos.x, cam_pos.y, cam_pos.z));

    // dynamic lighting from environment config
    GL_CALL(glUniform3f(u_light_dir_, -sun_dir.x, -sun_dir.y, -sun_dir.z));
    GL_CALL(glUniform3f(u_ambient_, ambient.x, ambient.y, ambient.z));
    GL_CALL(glUniform3f(u_sun_color_, sun_color.x, sun_color.y, sun_color.z));

    // fog
    GL_CALL(glUniform3f(u_fog_color_, fog_color.x, fog_color.y, fog_color.z));
    GL_CALL(glUniform1f(u_fog_density_, fog_density));

    // default base color (white)
    GL_CALL(glUniform4f(u_baseColor_, 1.0f, 1.0f, 1.0f, 1.0f));
}

void LambertianTechnique::setBaseColor(const math::Quat& color) {
    glUniform4f(u_baseColor_, color.x, color.y, color.z, color.w);
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

void LambertianTechnique::unbind() {
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glUseProgram(0));
}

} // namespace exd::render
