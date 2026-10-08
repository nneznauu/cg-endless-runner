# Libraries, assets, and assistance

| Component | Purpose | License / source |
|---|---|---|
| FreeGLUT (3.4 in cloud; teammate Windows setup uses 3.8) | Window, context, input | MIT-style; https://freeglut.sourceforge.net/ |
| GLAD 1 (generator 0.1.36 in cloud) | OpenGL 3.3 core loading, generated reproducibly from bundled Khronos specification | MIT plus Khronos generated-header notices; https://github.com/Dav1dde/glad/tree/v0.1.36 |
| GLM 0.9.9.8 | Matrices, vectors, LookAt, perspective | MIT or Happy Bunny; https://github.com/g-truc/glm/blob/master/copying.txt |
| Mesa / llvmpipe | Cloud software OpenGL driver, not a project runtime bundled asset | https://docs.mesa3d.org/license.html |
| CMake, Ninja | Build tooling | https://cmake.org/licensing/ ; https://github.com/ninja-build/ninja/blob/master/COPYING |
| python-docx, Pillow, Graphviz | DOCX generation, screenshot conversion, architecture diagram; not game dependencies | https://python-docx.readthedocs.io/ ; https://pillow.readthedocs.io/ ; https://graphviz.org/license/ |

Dependencies are installed externally, not vendored in this repository. Preserve their licence notices if distributing generated headers or binaries. No third-party models, textures or font images are included. Terrain height data, cube vertices, markings and bitmap glyphs are produced by project code. `docs/images/stage1-integrated.png` is captured from the current build. The older `daniyar-instancing.png`, architecture image and standalone report preserve historical evidence of the previous GLFW/GLEW prototype, whose libraries are no longer used by the integrated application.

OpenAI Codex substantially assisted the initial C++/GLSL implementation, tests, documentation and report draft. This is disclosed rather than attributed as unaided student work. The students must review, understand and adapt it and describe their actual contributions in the submitted report.
