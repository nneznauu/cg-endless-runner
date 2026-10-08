#pragma once

#include <glad/glad.h>
#include <filesystem>

GLuint loadProgram(const std::filesystem::path& vertexFile,
    const std::filesystem::path& fragmentFile);
