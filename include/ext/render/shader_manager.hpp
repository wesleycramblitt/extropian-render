#pragma once

#include <string>
#include <unordered_map>
#include <glad/gl.h>
#include <cstdint>

namespace ext::render {

class ShaderManager {
public:
    struct Program {
        GLuint id = 0;
        std::string vertex_path;
        std::string fragment_path;
    };

    ~ShaderManager();

    /// Load/cache a program by logical name. Hot-reloads if files changed.
    GLuint get_or_load(const std::string& name,
                       const std::string& vertex_path,
                       const std::string& fragment_path);
    void destroy_all();

private:
    std::unordered_map<std::string, Program> programs_;

    static std::string read_text_file(const std::string& path);
    static GLuint compile_shader(GLenum stage, const std::string& source, const std::string& debug_name);
    static GLuint link_program(GLuint vs, GLuint fs, const std::string& debug_name);
    void reload_program(const std::string& name, Program& p);
};

} // namespace ext::render
