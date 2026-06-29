#pragma once

#include <exd/render/mesh.hpp>
#include <exd/core/macros.hpp>
#include <glad/gl.h>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace exd::render {

/// GPU-side mesh: owns VAO, VBO, EBO. Constructed from a CPU Mesh.
struct MeshGPU {
    explicit MeshGPU(const Mesh& mesh) { generate(); upload(mesh); }

    ~MeshGPU() { destroy(); }
    MeshGPU(const MeshGPU&) = delete;
    MeshGPU& operator=(const MeshGPU&) = delete;
    MeshGPU(MeshGPU&& other) noexcept
        : vao(other.vao), vbo(other.vbo), ebo(other.ebo),
          topology(other.topology), index_count(other.index_count),
          vertex_count(other.vertex_count)
    {
        other.vao = other.vbo = other.ebo = 0;
        other.index_count = other.vertex_count = 0;
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLenum topology = GL_TRIANGLES;
    GLsizei index_count = 0;
    GLsizei vertex_count = 0;

    void upload(const Mesh& mesh) {
        switch (mesh.topology) {
            case Topology::Lines:    topology = GL_LINES;     break;
            case Topology::Points:   topology = GL_POINTS;    break;
            default:                 topology = GL_TRIANGLES; break;
        }
        index_count = static_cast<GLsizei>(mesh.indices.size());
        vertex_count = static_cast<GLsizei>(mesh.vertices.size());

        GL_CALL(glBindVertexArray(vao));
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
                             mesh.vertices.data(), GL_STATIC_DRAW));

        if (index_count > 0) {
            GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
            GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                                 mesh.indices.data(), GL_STATIC_DRAW));
        }

        // layout(location=0) vec3 a_pos;
        GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0));
        GL_CALL(glEnableVertexAttribArray(0));
        // layout(location=1) vec3 a_norm;
        GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      (void*)offsetof(Vertex, normal)));
        GL_CALL(glEnableVertexAttribArray(1));
        // layout(location=2) vec3 a_uv;
        GL_CALL(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      (void*)offsetof(Vertex, uv)));
        GL_CALL(glEnableVertexAttribArray(2));
        // layout(location=3) vec4 a_tangent;
        GL_CALL(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      (void*)offsetof(Vertex, tangent)));
        GL_CALL(glEnableVertexAttribArray(3));
        // layout(location=4) vec4 a_color;
        GL_CALL(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      (void*)offsetof(Vertex, color)));
        GL_CALL(glEnableVertexAttribArray(4));

        GL_CALL(glBindVertexArray(0));
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }

    void bind() const { GL_CALL(glBindVertexArray(vao)); }

    void generate() {
        GL_CALL(glGenVertexArrays(1, &vao));
        GL_CALL(glGenBuffers(1, &vbo));
        GL_CALL(glGenBuffers(1, &ebo));
    }

    void destroy() {
        if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    }
};

} // namespace exd::render
