#include <exd/render/graphics/techniques/highlight_technique.hpp>
#include <exd/core/macros.hpp>

namespace exd::render {

void HighlightTechnique::bind(const math::Mat4& view, const math::Mat4& proj,
                               const math::Vec3f& color) {
    program_ = ctx_.shader_manager.get_or_load(
        "highlight", "shaders/opengl/highlight/highlight.vert",
        "shaders/opengl/highlight/highlight.frag");
    GL_CALL(glUseProgram(program_));

    GLint vloc = glGetUniformLocation(program_, "u_view");
    GL_CALL(glUniformMatrix4fv(vloc, 1, GL_FALSE, view.m));
    GLint ploc = glGetUniformLocation(program_, "u_proj");
    GL_CALL(glUniformMatrix4fv(ploc, 1, GL_FALSE, proj.m));
    GLint cloc = glGetUniformLocation(program_, "u_color");
    GL_CALL(glUniform4f(cloc, color.x, color.y, color.z, 1.0f));

    // Wireframe overlay (desktop only — glPolygonMode unavailable in WebGL 2.0)
#ifndef __EMSCRIPTEN__
    GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    GL_CALL(glLineWidth(2.0f));
    GL_CALL(glEnable(GL_POLYGON_OFFSET_LINE));
    GL_CALL(glPolygonOffset(-1.0f, -1.0f));
#endif

    // No depth writes — highlight is overlay only
    GL_CALL(glDepthMask(GL_FALSE));
}

void HighlightTechnique::draw(uint32_t mesh_handle, const math::Mat4& model) {
    if (mesh_handle == 0) return;

    GLint loc = glGetUniformLocation(program_, "u_model");
    GL_CALL(glUniformMatrix4fv(loc, 1, GL_FALSE, model.m));

    const auto* mesh = ctx_.mesh_manager.bind(mesh_handle);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));
}

void HighlightTechnique::unbind() {
    GL_CALL(glDepthMask(GL_TRUE));
#ifndef __EMSCRIPTEN__
    GL_CALL(glDisable(GL_POLYGON_OFFSET_LINE));
    GL_CALL(glLineWidth(1.0f));
    GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
#endif
    GL_CALL(glUseProgram(0));
}

} // namespace exd::render
