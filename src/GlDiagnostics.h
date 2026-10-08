#pragma once

#include <glad/glad.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

inline const char* graphicsErrorName(GLenum code) {
    switch (code) {
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    default: return "UNKNOWN_GL_ERROR";
    }
}

// Called once, after the context and loader exist, before our rendering calls.
// A context library/loader may leave a legacy-query error in a core context.
// Log every existing flag so it cannot be mistaken for a later application error.
inline void reportStartupGraphicsErrors() {
    bool outOfMemory = false;
    for (GLenum code = glGetError(); code != GL_NO_ERROR; code = glGetError()) {
        std::cerr << "STARTUP WARNING (context/loader): " << graphicsErrorName(code)
            << " (" << code << "). Cleared before application initialization.\n";
        outOfMemory = outOfMemory || code == GL_OUT_OF_MEMORY;
    }
    if (outOfMemory) throw std::runtime_error("OpenGL ran out of memory during context startup.");
}

inline void checkGraphics(const char* stage) {
    std::ostringstream errors;
    bool failed = false;
    for (GLenum code = glGetError(); code != GL_NO_ERROR; code = glGetError()) {
        if (failed) errors << ", ";
        errors << graphicsErrorName(code) << " (" << code << ')';
        failed = true;
    }
    if (failed) {
        throw std::runtime_error(std::string("OpenGL error during ") + stage + ": " + errors.str());
    }
}
