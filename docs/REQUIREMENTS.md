# Requirements reviewed and midterm scope

Source: supplied `CG_Final_Project_Requirements_2026-2027.docx`, sections 2–4 and Topic 4 in section 6. The image in the request supplies the team work split. Lecture exercises are background material, not additional user requests to submit all weekly labs.

## Topic 4 must-implement list (copied from the assignment)

| Required wording | Current status | Next action |
|---|---|---|
| Forward-running character or vehicle with jump / lane-change / slide (at least one evasion control). | Done in Stage 1 | Mukhamejan's player: forward-scrolling world, jump, free horizontal movement. |
| Ground is a height-map (displacement in vertex shader or tessellation if available); tiling or scrolling so the run is endless. | Done in Stage 1 | Real GL_R32F height texture sampled in terrain VS, period 128 m. |
| Obstacles drawn with GPU instancing (one draw call per obstacle type, instance buffer with transforms). | Done in integrated scene | One cube type, pool 96, on-terrain filtering, mat4 buffer, one `glDrawElementsInstanced`; ON/OFF equivalence tested. |
| Collision against height-map and instances; game-over and restart. | Partial | Player ground contact and reset exist. Obstacle collision and game-over are missing. |
| Speed increases over time or by score; on-screen score. | Partial | Shared distance score implemented; speed fixed at 8 m/s. |

Technical expectation: `glDrawElementsInstanced / equivalent` — done; `height texture sampled in VS` — done and checked against CPU; `looping world origin to avoid float precision drift` — player stays near origin, obstacles recycle and shared terrain phase stays in [0,128).

## Common requirements

OpenGL 3.3 core (also explicitly tested with Mesa forced to 3.3), no game engine, external shaders, follow/orbit camera, resize-safe aspect ratio, depth test, CCW culling, lighting, movable light, on-screen help/state/score/team names, pause, both technique toggles and reset are present. Stage 1 starts running immediately. Windows Visual Studio project inputs are checked but MSVC is untested. ≥30 FPS on a lab GPU, a complete fail/game-over loop, final video, names/group verification and complete team report remain pending.

## Connection to every supplied teaching file

| Material | Relevant concepts used in the prototype |
|---|---|
| `L-1_Intro computer graphics Fundamentals.pdf` | Objects, viewer, light, vertex processing → rasterization → fragment processing. |
| `L-1-2_computer graphics fundmanetals.pdf` | Shader-based OpenGL; GLEW; VAO/VBO setup; buffer usage and updates. |
| `L-2-2 Computer Graphics Fundamentals.pdf` | Loading, compiling and linking shaders, attributes versus uniforms, double buffering, depth buffer. |
| `L-3-1_Input.pdf` | Event-driven interaction, keyboard callbacks, redraw and resize, now using the teammate's FreeGLUT application. |
| `L-3-2_Geometry.pdf` | Points versus vectors, homogeneous coordinates, shared world frames. |
| `L-4-1_Transformations.pdf` | Translation, scaling, right-to-left matrix composition. |
| `L-4-2_Models.pdf` | Indexed polygon meshes, CCW outward faces, separate geometry/topology. |
| `L-5-1_Viewing.pdf` | LookAt, perspective, homogeneous clip coordinates, aspect ratio. |
| `L-5-2_Shading.pdf` | Diffuse and specular lighting, normalized vectors, halfway vector and per-fragment shading. |
| `Practice_Session_ 4 Transformations.pptx` | GLM identity, `M = T * S`, column-major upload with `GL_FALSE`; a `mat4` becomes four instance attributes. |
| `Practice_Session_Coordinates_Camera_Colors (1) (1).pptx` | `P * V * M * p`, view inverse, camera basis, delta time, color/material interaction. |

Some older lecture examples use fixed-function calls or the Angel `mat.h` convention. This project follows the 3.3 core requirement and the GLM practice sessions. GLM uses column-major matrices, so uniform upload uses `GL_FALSE`; the older slide instruction to transpose does not apply to this storage convention. Default GLM construction is not relied on: every model starts explicitly from `glm::mat4(1.0f)`.
