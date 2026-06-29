#include <exd/render/shader_manager.hpp>
#include <exd/core/macros.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace exd::render {

ShaderManager::~ShaderManager() { destroy_all(); }

void ShaderManager::destroy_all() {
    for (auto& [_, p] : programs_) {
        if (p.id != 0) glDeleteProgram(p.id);
    }
    programs_.clear();
}

GLuint ShaderManager::get_or_load(const std::string& name,
                                   const std::string& vertex_path,
                                   const std::string& fragment_path) {
    auto it = programs_.find(name);
    if (it != programs_.end()) return it->second.id;

    Program p;
    p.vertex_path = vertex_path;
    p.fragment_path = fragment_path;
    reload_program(name, p);
    programs_.emplace(name, p);
    return p.id;
}

void ShaderManager::reload_program(const std::string& name, Program& p) {
    const std::string vs_src = read_text_file(p.vertex_path);
    const std::string fs_src = read_text_file(p.fragment_path);

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src, name + " (VS)");
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src, name + " (FS)");
    GLuint prog = link_program(vs, fs, name);

    if (p.id != 0) glDeleteProgram(p.id);
    p.id = prog;
    glDeleteShader(vs);
    glDeleteShader(fs);
}

std::string ShaderManager::read_text_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Failed to open shader file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint ShaderManager::compile_shader(GLenum stage, const std::string& source, const std::string& debug_name) {
    GLuint sh = glCreateShader(stage);
    const char* src = source.c_str();
    GL_CALL(glShaderSource(sh, 1, &src, nullptr));
    GL_CALL(glCompileShader(sh));

    GLint ok = 0;
    GL_CALL(glGetShaderiv(sh, GL_COMPILE_STATUS, &ok));
    if (!ok) {
        GLint len = 0;
        GL_CALL(glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len));
        std::string log(static_cast<size_t>(len), '\0');
        GL_CALL(glGetShaderInfoLog(sh, len, nullptr, log.data()));
        glDeleteShader(sh);
        throw std::runtime_error("Shader compile failed: " + debug_name + "\n" + log);
    }
    return sh;
}

GLuint ShaderManager::link_program(GLuint vs, GLuint fs, const std::string& debug_name) {
    GLuint p = glCreateProgram();
    GL_CALL(glAttachShader(p, vs));
    GL_CALL(glAttachShader(p, fs));
    GL_CALL(glLinkProgram(p));

    GLint ok = 0;
    GL_CALL(glGetProgramiv(p, GL_LINK_STATUS, &ok));
    if (!ok) {
        GLint len = 0;
        GL_CALL(glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len));
        std::string log(static_cast<size_t>(len), '\0');
        GL_CALL(glGetProgramInfoLog(p, len, nullptr, log.data()));
        glDeleteProgram(p);
        throw std::runtime_error("Program link failed: " + debug_name + "\n" + log);
    }
    return p;
}

} // namespace exd::render
