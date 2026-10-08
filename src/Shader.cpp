#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    std::string readText(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) throw std::runtime_error("Cannot open shader: " + path.string());
        std::ostringstream result;
        result << input.rdbuf();
        return result.str();
    }

    GLuint compile(GLenum type, const std::filesystem::path& path) {
        const std::string text = readText(path);
        const char* source = text.c_str();
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> log(static_cast<size_t>(length) + 1, '\0');
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error(path.string() + ":\n" + log.data());
        }
        return shader;
    }
}

GLuint loadProgram(const std::filesystem::path& vertexFile,
    const std::filesystem::path& fragmentFile, const char* feedbackVarying) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexFile);
    GLuint fragment = 0;
    try { fragment = compile(GL_FRAGMENT_SHADER, fragmentFile); }
    catch (...) { glDeleteShader(vertex); throw; }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (feedbackVarying) glTransformFeedbackVaryings(program, 1, &feedbackVarying, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<size_t>(length) + 1, '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error("Shader link error:\n" + std::string(log.data()));
    }
    return program;
}
