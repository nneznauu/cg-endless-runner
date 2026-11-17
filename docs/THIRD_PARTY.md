# Libraries, assets, and assistance

| Component | Purpose | License / source |
|---|---|---|
| FreeGLUT (3.4 in cloud; teammate Windows setup uses 3.8) | Window, context, input | MIT-style; https://freeglut.sourceforge.net/ |
| GLAD 1 (generator 0.1.36 in cloud) | OpenGL 3.3 core loading, generated reproducibly from bundled Khronos specification | MIT plus Khronos generated-header notices; https://github.com/Dav1dde/glad/tree/v0.1.36 |
| GLM 0.9.9.8 | Matrices, vectors, LookAt, perspective | MIT or Happy Bunny; https://github.com/g-truc/glm/blob/master/copying.txt |
| Mesa / llvmpipe | Cloud software OpenGL driver, not a project runtime bundled asset | https://docs.mesa3d.org/license.html |
| CMake, Ninja | Build tooling | https://cmake.org/licensing/ ; https://github.com/ninja-build/ninja/blob/master/COPYING |
| python-docx, Pillow, Graphviz | DOCX generation, screenshot conversion, architecture diagram; not game dependencies | https://python-docx.readthedocs.io/ ; https://pillow.readthedocs.io/ ; https://graphviz.org/license/ |

The original Visual Studio setup used external dependencies; the final CMake path below includes the generated GLAD loader and fetches the remaining libraries. Preserve their licence notices if distributing generated headers or binaries. No third-party models, textures or font images are included. Terrain height data, cube vertices, markings and bitmap glyphs are produced by project code. `docs/images/stage1-integrated.png` is a historical Stage 1 capture. The older `daniyar-instancing.png`, architecture image and standalone report preserve historical evidence of the previous GLFW/GLEW prototype, whose libraries are no longer used by the integrated application.


## Final CMake build

The final build includes the generated GLAD 1 loader and its notices in `third_party/glad`; see `ORIGIN.md` there for the generator command and source commit. GLM 1.0.1 and FreeGLUT 3.4.0 are fetched from their official repositories at the exact commits and SHA-256 hashes in `cmake/RunnerDependencies.cmake`. They are build dependencies, not student-authored code. FreeGLUT is linked statically. The Windows helper copies all three licence notices next to the executable.

Original sources:
- https://github.com/Dav1dde/glad/tree/1ecd45775d96f35170458e6b148eb0708967e402
- https://github.com/g-truc/glm/tree/0af55ccecd98d4e5a8d1fad7de25ba429d60e863
- https://github.com/freeglut/freeglut/tree/63c0e78d84ecbfcf8accd8bac04404427899053b
