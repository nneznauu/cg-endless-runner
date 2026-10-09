#include "Obstacles.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <cmath>
#include <stdexcept>

namespace runner {
void Obstacles::initialize() {
    struct Vertex { glm::vec3 position, normal; };
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const std::array<glm::vec3, 6> normals = {{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    const std::array<glm::vec3, 6> axes = {{{0,0,-1},{0,0,1},{1,0,0},{1,0,0},{1,0,0},{-1,0,0}}};
    for (std::size_t f = 0; f < normals.size(); ++f) {
        glm::vec3 n = normals[f], u = axes[f], v = glm::cross(n,u);
        const unsigned int start = static_cast<unsigned int>(vertices.size());
        for (const auto& corner : std::array<glm::vec2,4>{{{-1,-1},{1,-1},{1,1},{-1,1}}})
            vertices.push_back({0.5f * (n + corner.x*u + corner.y*v), n});
        for (unsigned int i : {0u,1u,2u,0u,2u,3u}) indices.push_back(start+i);
    }
    glGenVertexArrays(1,&vao_); glBindVertexArray(vao_);
    glGenBuffers(1,&mesh_); glBindBuffer(GL_ARRAY_BUFFER,mesh_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glGenBuffers(1,&indices_); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,static_cast<GLsizeiptr>(indices.size()*sizeof(unsigned int)),indices.data(),GL_STATIC_DRAW);
    glGenBuffers(1,&instances_); glBindBuffer(GL_ARRAY_BUFFER,instances_);
    static_assert(sizeof(glm::mat4)==16*sizeof(float), "Packed mat4 expected");
    for (GLuint col=0; col<4; ++col) {
        glEnableVertexAttribArray(2+col);
        glVertexAttribPointer(2+col,4,GL_FLOAT,GL_FALSE,sizeof(glm::mat4),reinterpret_cast<void*>(col*sizeof(glm::vec4)));
        glVertexAttribDivisor(2+col,1);
    }
    glBindVertexArray(0);
}
void Obstacles::shutdown() {
    glDeleteBuffers(1,&instances_); glDeleteBuffers(1,&indices_);
    glDeleteBuffers(1,&mesh_); glDeleteVertexArrays(1,&vao_);
    instances_=indices_=mesh_=vao_=0;
    transforms_.clear();
}
void Obstacles::upload(const std::vector<Obstacle>& obstacles, const GroundHeight& height) {
    transforms_.clear(); transforms_.reserve(obstacles.size());
    for (const auto& o : obstacles) {
        const float y = height(o.x,o.z);
        if (!std::isfinite(y)) throw std::runtime_error("Ground sampler returned non-finite height");
        glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(o.x,y+o.height*0.5f,o.z));
        model = glm::scale(model,glm::vec3(o.width,o.height,o.depth));
        transforms_.push_back(model);
    }
    glBindBuffer(GL_ARRAY_BUFFER,instances_);
    const auto bytes = static_cast<GLsizeiptr>(transforms_.size()*sizeof(glm::mat4));
    glBufferData(GL_ARRAY_BUFFER,bytes,nullptr,GL_STREAM_DRAW);
    if (bytes) glBufferSubData(GL_ARRAY_BUFFER,0,bytes,transforms_.data());
}
std::size_t Obstacles::draw(GLuint program, bool instanced) const {
    glBindVertexArray(vao_);
    glUniform1i(glGetUniformLocation(program,"uInstanced"),instanced?1:0);
    if (transforms_.empty()) return 0;
    if (instanced) {
        glDrawElementsInstanced(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(transforms_.size()));
        return 1;
    }
    for (const auto& model : transforms_) {
        glUniformMatrix4fv(glGetUniformLocation(program,"uModel"),1,GL_FALSE,glm::value_ptr(model));
        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr);
    }
    return transforms_.size();
}
void Obstacles::validateLayout() const {
    glBindVertexArray(vao_);
    for (GLuint i=2;i<6;++i) {
        GLint divisor=0,enabled=0,size=0;
        glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_DIVISOR,&divisor);
        glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&enabled);
        glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_SIZE,&size);
        if (divisor!=1 || !enabled || size!=4) throw std::runtime_error("Invalid instance attribute layout");
    }
}
}
