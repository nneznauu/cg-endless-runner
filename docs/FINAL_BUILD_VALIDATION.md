# Final build preparation validation

Validated on 17 November 2026 (UTC+5). Gameplay baseline: `c02f494`, including the normal game and stress mode. This branch adds build tools, a performance exporter and documentation; it does not replace the team's gameplay or shader implementations.

## Environment and build

- Debian 13, GCC 14.2, CMake 3.31.6, Ninja 1.11.1.
- Bundled GLAD 0.1.36, GLM 1.0.1, static FreeGLUT 3.4.0.
- A separate build downloaded the official GLM/FreeGLUT archives through FetchContent and verified their pinned SHA-256 hashes. Configuration and compilation passed without local source overrides.
- OpenGL 4.5 core, Mesa 25.0.7, llvmpipe software renderer, Xvfb at 1280 by 720. This is not a Windows lab GPU.

## Results

Both CTest executables passed: 304 game assertions and 193 terrain/player assertions, 497 total.

The integrated OpenGL smoke test passed with no GL errors. It checked Ready, start, collisions, Game Over, Enter/R restart, best score, jump/landing, pause, height-map and instancing toggles, orbit/follow camera, light movement, resize and bounded scrolling. The actual terrain vertex shader matched CPU sampling at 17,297 vertices for five phase/displacement cases.

Normal-scene instancing used 3 calls instead of 12, with zero mismatched pixels under the test tolerance. Stress mode submitted 1000 obstacles, used 3 calls instead of 1000, and also matched pixel-for-pixel under the tolerance. N/M/B transitions and returning to the normal game passed.

The PowerShell performance exporter ran successfully under PowerShell 7.4.6, including three real ON/OFF runs and CSV, JSON and Markdown output. All 1000 requested instances were submitted in both modes.

| Software-renderer check only | Submitted | Obstacle draws | Median render time, ms | FPS derived from median time |
|---|---:|---:|---:|---:|
| Instancing ON | 1000 | 3 | 7.899 | 126.598 |
| Instancing OFF | 1000 | 1000 | 8.851 | 112.982 |

These timings cover a fixed stress scene with 20 warmup and 120 measured frames per mode, three repetitions, and `glFinish` after each frame. They exclude game updates, instance upload, HUD and buffer swapping. They validate the measurement workflow; they are not the final hardware performance table and do not establish normal gameplay FPS on Windows.

## Checks remaining on the target computer

The user subsequently built successfully with MSVC 19.44 on Windows and passed CTest. The OpenGL check on Intel Arc Graphics, OpenGL 3.3.0, driver build 32.0.101.6314, stopped at `Terrain CPU/GPU height mismatch`. The portable build path therefore works on that PC; the graphics check still needs to pass after the correction below. Then run `-Verify -Benchmark` for the actual GPU table, check normal gameplay at 1280 by 720, capture the final screenshots and record the demo. Use the Windows guide for full commands.

## Terrain sampling precision correction

The previous vertex shader delegated bilinear interpolation to `textureLod`, while the CPU interpolated in floating point. Hardware filtering may quantize interpolation weights. This is a plausible explanation for the Intel failure, not a confirmed driver diagnosis: the original message did not print the differing values.

The shader now fetches the four R32F texels directly and uses the CPU's interpolation formula, including X clamping and Z wrapping. Validation keeps its original 0.0001-metre threshold, additionally checks X/Z against the input mesh, and prints the phase, vertex, expected/GPU positions and error if a mismatch occurs. Successful cases print their maximum height error.

The rebuilt code passed both CTest tests (497 assertions) and the full GL smoke test in an OpenGL 3.3 / GLSL 330 Mesa context. All five terrain cases reported zero height error across 17,297 vertices. A temporary shader variant rounding weights to steps of 1/256 reproduced a rejection at phase 12.225: CPU height 0.772213936, GPU height 0.772111535, difference 0.00010240078. This negative control was not included in the game. The corrected shader then passed at the unchanged threshold.

Intel Arc verification remains pending. The software-renderer timing table above predates this shader change; regenerate performance measurements for the corrected build.

References: [GLSL 3.30, textureSize and texelFetch](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.3.30.pdf); [Khronos discussion of filtering-weight quantization](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_texture_filter_minmax.txt), issue 4.
