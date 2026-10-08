# cg-endless-runner

Topic 4: **Endless runner with instanced obstacles and a height-map ground**.
Astana IT University, Computer Graphics Fundamentals, 2026–2027.

This is Stage 1, the terrain/player contribution for a two-person team. It is an initial midterm prototype, not a complete final submission.

## Build and run

- OS: Windows 10/11 x64.
- Compiler: MSVC v143, Visual Studio 2022, C++17 and Windows SDK.
- Rendering: OpenGL 3.3 core.
- Libraries: GLAD 1 (`glad/glad.h`, `glad.c`), FreeGLUT 3.8.0 (x64 Release shared library), GLM 1.0.3.

Open `EndlessRunner.sln`, choose `Release | x64`, then `Ctrl + F5`. See `START_HERE_RU.md` for the one-page build guide and DLL troubleshooting.

From a Visual Studio developer command prompt:

```bat
msbuild EndlessRunner.sln /m /p:Configuration=Release /p:Platform=x64
bin\Release\EndlessRunner.exe
```

Library paths are centralized in `Dependencies.props`. The defaults match the uploaded lab project. For a different PC, set `GladRoot`, `GlmRoot`, `FreeglutRoot` and `FreeglutLibDir` to that PC's installations, or supply matching dependencies under `external` (see the conditions in `Dependencies.props`). The dependency packages themselves are not included in this archive; the original upload contained project configuration and source only. Preserve their licenses when packaging them for submission.

Shaders are loaded from separate files. Build copies shaders and the FreeGLUT DLL when found to the executable directory. Missing dependencies and shader failures produce errors.

Extract the complete ZIP with its `src` and `shaders` directories. Source paths are anchored to the project directory; the project also accepts all source/header files in the project root. Shader files can be in `shaders` or the project root and are copied to the executable's `shaders` directory.

Existing context/loader OpenGL error flags are logged and cleared once before application initialization. Errors in the application's own shader, mesh, texture and rendering stages are checked separately and reported with the stage name. This prevents an earlier core-context compatibility query from being attributed to terrain initialization.

## Exact controls

Click the game window before testing. On Windows, letter controls use virtual-key codes instead of translated characters, so the same key positions work with English and Russian layouts. Toggles run once per press; movement and light rotation continue while held. Letter states and held arrows/Space are cleared when the game loses focus. Other platforms keep the GLUT character callbacks.

| Key | Action |
| --- | --- |
| Space | Jump when grounded |
| A / D | Move horizontally |
| P | Pause/resume world movement and player physics |
| R | Reset player position and travelled distance |
| T | Toggle height-map displacement |
| C | Follow/orbit camera |
| Arrow keys | Rotate orbit camera |
| Mouse wheel | Change orbit-camera distance |
| J / L | Rotate the directional light |
| V | Toggle wireframe |
| H | Show/hide help |
| Esc | Exit |

## Stage 1 status

| Requirement | Status |
| --- | --- |
| Height texture sampled in vertex shader | Implemented |
| Endless terrain scrolling with bounded coordinates | Implemented |
| Player jump and horizontal input | Implemented |
| Player ground contact against the displaced mesh | Implemented |
| Perspective camera, resize, orbit controls | Implemented |
| Lighting, depth testing, back-face culling | Implemented |
| Pause, displacement toggle, help and distance HUD | Implemented |
| GPU-instanced obstacles | Pending Daniyar's module |
| Obstacle collisions, game over and restart after failure | Pending |
| Speed progression | Pending; Stage 1 speed is constant |
| Full team names, final report, performance table and video | Pending |

The height texture is generated at startup from periodic functions and uploaded as a real `GL_R32F` texture. It is not an image pasted onto a flat mesh. The vertex shader samples it with `textureLod` and modifies vertex height. Texture coordinates repeat in the running direction; local scene coordinates stay bounded. See `SHADER_NOTES_RU.md`.

## Module ownership plan

| Member | Planned responsibility |
| --- | --- |
| Mukhamejan Abdikerim | HeightField, terrain shaders, player, camera, corresponding report sections |
| Daniyar (confirm full name against topic list) | GPU-instanced obstacles, spawning and game state, corresponding report sections |
| Both | Application integration, renderer changes, verification and shader explanation at defence |

This table is a plan. The report must describe actual reviewed and completed contributions.

## Sources, assets and assistance

No external models, texture images or fonts are required. Terrain data and HUD glyphs are produced by this source. Shader files were newly drafted for this project rather than copied from a tutorial or another team. Standard OpenGL interfaces and mathematical operations are shared techniques.

**AI assistance disclosure:** The initial Stage 1 scaffold, shaders, terrain sampling, player update and supporting documentation were generated with OpenAI ChatGPT/Codex on October 8, 2026. The team must review, understand and adapt this work and disclose substantial generated fragments in the report, as required by the assignment. A generated starter is not evidence of completed personal authorship.

Primary references for APIs and libraries:

- Khronos Group. (n.d.). *OpenGL registry*. https://registry.khronos.org/OpenGL/
- FreeGLUT Programming Consortium. (2013). *The freeglut application programming interface*. https://freeglut.sourceforge.net/docs/api.php
- Microsoft. (n.d.). *.vcxproj and .props file structure*. Microsoft Learn. https://learn.microsoft.com/en-us/cpp/build/reference/vcxproj-file-structure
- Microsoft. (n.d.). *GetAsyncKeyState function*. Microsoft Learn. https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate
- G-Truc Creation. (n.d.). *OpenGL Mathematics*. https://github.com/g-truc/glm
- Herberth, D. (n.d.). *GLAD*. https://github.com/Dav1dde/glad
