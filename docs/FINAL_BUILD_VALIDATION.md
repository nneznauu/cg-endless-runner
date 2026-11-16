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

Run `scripts/build-windows.ps1 -Verify -Run` in Windows PowerShell. The Visual Studio discovery and native MSVC build path have been reviewed but cannot be executed on this Linux machine. Then run `-Verify -Benchmark` for the actual GPU table, check normal gameplay at 1280 by 720, capture the final screenshots and record the demo. Use the Windows guide for full commands.
