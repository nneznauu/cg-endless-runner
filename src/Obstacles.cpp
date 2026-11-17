#include "Obstacles.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace runner {
glm::vec3 Obstacles::color(int type) {
    switch (type) {
    case 0: return {0.98f, 0.80f, 0.20f};
    case 1: return {0.20f, 0.72f, 0.78f};
    default: return {0.86f, 0.28f, 0.42f};
    }
}
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
    glGenBuffers(1,&mesh_); glBindBuffer(GL_ARRAY_BUFFER,mesh_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),vertices.data(),GL_STATIC_DRAW);
    glGenBuffers(1,&indices_);
    static_assert(sizeof(glm::mat4)==16*sizeof(float), "Packed mat4 expected");
    static_assert(sizeof(Instance)==19*sizeof(float), "Packed instance expected");
    for (std::size_t t = 0; t < batches_.size(); ++t) {
        Batch& batch = batches_[t];
        glGenVertexArrays(1,&batch.vao); glBindVertexArray(batch.vao);
        glBindBuffer(GL_ARRAY_BUFFER,mesh_);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices_);
        if (t == 0)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,static_cast<GLsizeiptr>(indices.size()*sizeof(unsigned int)),indices.data(),GL_STATIC_DRAW);
        glGenBuffers(1,&batch.buffer); glBindBuffer(GL_ARRAY_BUFFER,batch.buffer);
        for (GLuint col=0; col<4; ++col) {
            glEnableVertexAttribArray(2+col);
            glVertexAttribPointer(2+col,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(col*sizeof(glm::vec4)));
            glVertexAttribDivisor(2+col,1);
        }
        glEnableVertexAttribArray(ColorLocation);
        glVertexAttribPointer(ColorLocation,3,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(sizeof(glm::mat4)));
        glVertexAttribDivisor(ColorLocation,1);
    }
    glBindVertexArray(0);
}
void Obstacles::shutdown() {
    for (Batch& batch : batches_) {
        glDeleteBuffers(1,&batch.buffer); glDeleteVertexArrays(1,&batch.vao);
        batch.buffer = batch.vao = 0;
        batch.instances.clear();
    }
    glDeleteBuffers(1,&indices_); glDeleteBuffers(1,&mesh_);
    indices_ = mesh_ = 0;
}
void Obstacles::upload(const std::vector<Obstacle>& obstacles, const GroundHeight& height) {
    for (Batch& batch : batches_) batch.instances.clear();
    for (const auto& o : obstacles) {
        if (o.type < 0 || o.type >= Game::TypeCount) throw std::runtime_error("Unknown obstacle type");
        const float y = height(o.x,o.z);
        if (!std::isfinite(y)) throw std::runtime_error("Ground sampler returned non-finite height");
        glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(o.x,y+o.height*0.5f,o.z));
        model = glm::scale(model,glm::vec3(o.width,o.height,o.depth));
        batches_[static_cast<std::size_t>(o.type)].instances.push_back({model,color(o.type)});
    }
    for (const Batch& batch : batches_) {
        glBindBuffer(GL_ARRAY_BUFFER,batch.buffer);
        const auto bytes = static_cast<GLsizeiptr>(batch.instances.size()*sizeof(Instance));
        glBufferData(GL_ARRAY_BUFFER,bytes,nullptr,GL_STREAM_DRAW);
        if (bytes) glBufferSubData(GL_ARRAY_BUFFER,0,bytes,batch.instances.data());
    }
}
std::size_t Obstacles::count() const {
    std::size_t total = 0;
    for (const Batch& batch : batches_) total += batch.instances.size();
    return total;
}
std::size_t Obstacles::count(int type) const {
    if (type < 0 || type >= Game::TypeCount) return 0;
    return batches_[static_cast<std::size_t>(type)].instances.size();
}
std::size_t Obstacles::drawCalls(bool instanced) const {
    if (!instanced) return count();
    std::size_t calls = 0;
    for (const Batch& batch : batches_) if (!batch.instances.empty()) ++calls;
    return calls;
}
std::size_t Obstacles::draw(GLuint program, bool instanced) const {
    glUniform1i(glGetUniformLocation(program,"uInstanced"),instanced?1:0);
    const GLint modelLocation = glGetUniformLocation(program,"uModel");
    const GLint typeLocation = glGetUniformLocation(program,"uType");
    std::size_t calls = 0;
    for (std::size_t t = 0; t < batches_.size(); ++t) {
        const Batch& batch = batches_[t];
        if (batch.instances.empty()) continue;
        glUniform1i(typeLocation,static_cast<GLint>(t));
        glBindVertexArray(batch.vao);
        if (instanced) {
            glDrawElementsInstanced(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(batch.instances.size()));
            ++calls;
            continue;
        }
        glDisableVertexAttribArray(ColorLocation);
        for (const Instance& instance : batch.instances) {
            glVertexAttrib3fv(ColorLocation,glm::value_ptr(instance.color));
            glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(instance.model));
            glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr);
            ++calls;
        }
        glEnableVertexAttribArray(ColorLocation);
    }
    glBindVertexArray(0);
    return calls;
}
void Obstacles::validateLayout() const {
    for (const Batch& batch : batches_) {
        glBindVertexArray(batch.vao);
        for (GLuint i=2;i<=ColorLocation;++i) {
            GLint divisor=0,enabled=0,size=0,stride=0;
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_DIVISOR,&divisor);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&enabled);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_SIZE,&size);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_STRIDE,&stride);
            const GLint expected = i==ColorLocation ? 3 : 4;
            if (divisor!=1 || !enabled || size!=expected || stride!=static_cast<GLint>(sizeof(Instance)))
                throw std::runtime_error("Invalid instance attribute layout");
        }
        GLint buffer=0;
        glGetVertexAttribiv(ColorLocation,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&buffer);
        if (static_cast<GLuint>(buffer)!=batch.buffer) throw std::runtime_error("Instance colour must come from the type buffer");
    }
    glBindVertexArray(0);
}
}
