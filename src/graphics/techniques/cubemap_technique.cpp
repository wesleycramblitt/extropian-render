#include <exd/render/graphics/techniques/cubemap_technique.hpp>
#include <exd/core/macros.hpp>
#include <cstdio>

namespace exd::render {

void CubeMapRenderTechnique::bind() {
    cubemap_program_ = ctx_.shader_manager.get_or_load(
        "cubemap", "shaders/opengl/cubemap/cubemap.vert", "shaders/opengl/cubemap/cubemap.frag");
    GL_CALL(glDepthFunc(GL_LEQUAL));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDisable(GL_CULL_FACE));
    GL_CALL(glUseProgram(cubemap_program_));
}

void CubeMapRenderTechnique::draw(const Renderable& renderable) {
    GLint u_skybox = glGetUniformLocation(cubemap_program_, "u_skybox");
    GLint u_view = glGetUniformLocation(cubemap_program_, "u_view");
    GLint u_proj = glGetUniformLocation(cubemap_program_, "u_proj");
    GLint u_model = glGetUniformLocation(cubemap_program_, "u_model");

    // Skybox cube is already in world space centred at origin;
    // model matrix is identity — the cubemap rotates with the camera
    // via the view matrix (translation stripped for infinite projection).
    math::Mat4 identity = math::Mat4::identity();
    GL_CALL(glUniformMatrix4fv(u_model, 1, GL_FALSE, identity.m));

    // Strip translation from view matrix so skybox appears infinitely distant.
    math::Mat4 view_no_trans = std::get<math::Mat4>(renderable.uniforms.at("u_view"));
    view_no_trans.m[12] = 0.0f;  // column 3, row 0 (X translation)
    view_no_trans.m[13] = 0.0f;  // column 3, row 1 (Y translation)
    view_no_trans.m[14] = 0.0f;  // column 3, row 2 (Z translation)

    GL_CALL(glUniformMatrix4fv(u_view, 1, GL_FALSE, view_no_trans.m));
    GL_CALL(glUniformMatrix4fv(u_proj, 1, GL_FALSE, std::get<math::Mat4>(renderable.uniforms.at("u_proj")).m));

    if (renderable.mesh_handle == 0 || renderable.texture_handle == 0) {
        // SKIP: mesh_handle or texture_handle is 0
        return;
    }

    // Bind cubemap directly
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, renderable.texture_handle));
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

} // namespace exd::render
