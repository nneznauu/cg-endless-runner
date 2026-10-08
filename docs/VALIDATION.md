# Validation of Daniyar's prototype

**Historical standalone build. For the integrated Stage 1 on FreeGLUT/GLAD, use [STAGE1_INTEGRATED_VALIDATION.md](STAGE1_INTEGRATED_VALIDATION.md). Counts and timings below belong to the earlier GLFW/GLEW prototype.**

Run date: 8 October 2026. Linux Debian 13, GCC 14.2.0, CMake 3.31.6, GLFW 3.4, GLEW 2.2, GLM 0.9.9.8. OpenGL reports **4.5 core, Mesa 25.0.7**, GLSL source targets **330 core**. Renderer: **llvmpipe (LLVM 19.1.7, 256 bits)**, software rendering on Intel Xeon Platinum 8573C, 5 logical CPUs visible.

## Executed checks

| Check | Result |
|---|---|
| CMake Release build, GCC warnings enabled | Passed |
| CTest `game_logic` | 1 test executed, passed; 212 runtime assertions (not compiled out in Release) |
| Ready state and pause | Distance and positions do not advance |
| Distance and movement | 12 m/s integrated with delta time; motion is +Z |
| Recycling/reset | Constant pool, bounded positions, deterministic initial state restored |
| Frame-rate independence | 75 simulated seconds at 30 vs 144 updates/s; matching distance and positions |
| Long-run precision | 10,000 simulated one-hour steps with 3 obstacles; 432,000,000 m distance and bounded render/texture coordinates |
| Invalid configuration/time | Zero instances, negative dt, infinity and NaN rejected |
| OpenGL smoke | Passed; four mat4 columns have divisor 1 |
| Instanced vs individual rendering | 1 vs 96 obstacle draw calls; zero pixels differ beyond a tolerance of 1 channel value |
| Visibility | Removing obstacles changes 8,489 pixels in the fixed 1280×720 scene |
| Movement and light | Each independently changes more than 100 pixels |
| Resize and HUD | Render/readback at 960×540 and restored 1280×720; no GL error |
| Shader failure diagnostics | Missing shader and deliberately invalid external GLSL both print errors and exit 1 |
| Rootless cloud setup | Complete setup script passed twice; startup helper creates and cleans up its own Xvfb |

The smoke scene is rendered without HUD for the equivalence comparison, so changing the ON/OFF text cannot hide an image mismatch. The final screenshot includes the HUD and comes from the same executable. CPU simulation tests validate elapsed simulation time; they are not a one-minute video or a manual play-through of a complete game.

## Measured completed-frame timings

| Submitted obstacles | Mode | Obstacle draws | Frame time, ms | FPS |
|---|---|---|---|---|
| 96 | Instancing ON | 1 | 2.967 | 337.033 |
| 96 | Instancing OFF | 96 | 3.378 | 296.017 |
| 1000 | Instancing ON | 1 | 3.609 | 277.085 |
| 1000 | Instancing OFF | 1000 | 3.980 | 251.227 |

Method: 1280×720; Release; fixed simulation state at 1 second; 20 warmup + 120 measured frames per mode; VSync off; HUD off; `glFinish` after each frame; timer is `std::chrono::steady_clock`. Transforms are uploaded once before timing. The common floor and marker are included. There is no CPU frustum culling, so some submitted instances lie beyond the far plane; the stress test increases submitted instances without making all 1000 visible at once. These are whole-frame wall timings on a software renderer, not GPU query times and not the full animated update/upload loop.

This final run shows lower frame times with fewer draw submissions for both counts. Earlier exploratory runs did not consistently show a benefit at 96 instances. The workload is small, and cloud CPU scheduling, cache state and run order affect results; do not infer a universal speedup or compare the 96/1000 rows as controlled scaling measurements. The table reports one final run rather than selecting the best sample. Repeat on the actual lab GPU to establish the ≥30 FPS requirement there. Raw values are retained in `docs/performance.csv`.

## Reproduce

```sh
bash scripts/setup-cloud.sh
bash scripts/cloud-run.sh --smoke-test --screenshot artifacts/cloud-smoke.ppm
bash scripts/cloud-run.sh --benchmark
bash scripts/cloud-run.sh --instances 1000 --benchmark
```

On a desktop use `./build/endless_runner` directly after the README build. Optional `--shader-dir` selects external shader files; missing or invalid shaders must produce a diagnostic and a nonzero process exit.

## Not verified / not implemented

Windows compilation and hardware GPU performance were not tested. No height-map VS/CPU sampler, player evasion, collision/game-over, increasing speed, final integrated scene or video exists yet. The local implementation supports development of Daniyar's module; these missing features prevent claiming completion of the final assignment.
