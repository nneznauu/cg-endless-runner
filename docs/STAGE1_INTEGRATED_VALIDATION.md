# Integrated Stage 1 — validation

Base remote commit: `b461826` (both source and shader files now present). Local integration uses FreeGLUT/GLAD and a single canonical `src/` + `shaders/` tree. The old blocker in `STAGE1_REVIEW_RU.md` is resolved.

## Executed checks

| Check | Result |
|---|---|
| Release CMake build, GCC 14.2, warnings enabled | Passed |
| `game_logic` | Passed, 212 assertions: shared 8 m/s movement/distance, pause, reset, wrap, 75-second frame-rate independence, long-run precision |
| `terrain_player_logic` | Passed, 193 assertions: real height data, 128 m periodicity, displacement OFF, both mesh triangles' interpolation, jump/landing, horizontal limits, flat-ground contact |
| Integrated rendering | Passed on Mesa OpenGL 4.5 core and with `MESA_GL_VERSION_OVERRIDE=3.3`, GLSL 330 |
| Actual terrain vertex shader vs CPU | 17,297 transformed vertices in each of 5 phase/displacement cases; Y error at most 0.0001 m |
| Instancing ON/OFF | 1 vs 25 obstacle draws at the reference frame; zero mismatched pixels beyond 1/255 channel tolerance |
| Obstacle visibility | Removing the obstacle batch changes 9,146 pixels at 1280×720 |
| P and R through actual action handlers | Pause freezes player, obstacles, phase and distance; reset restores them and preserves pause |
| T and I through actual action handlers | Terrain displacement and instancing modes switch successfully |
| Space / player / camera | Jump, landing on the mesh, horizontal input, C orbit/follow and orbit rotation checked |
| Scrolling/recycling | 17 simulated seconds cross the 128 m phase boundary and recycle obstacles; coordinates remain bounded |
| Light and resize | Light changes image; real window resize 960×540 and back to 1280×720 succeeds |
| GL diagnostics | No application GL errors in the checked frames |
| Rootless setup/start scripts | Completed installation/build/CTest/GL smoke with FreeGLUT and reproducibly generated GLAD 1 |
| Visual Studio project inspection | Every local source/header/shader/document input exists, no duplicate sources, exactly one main.cpp |

Both CTest tests executed: **405 assertions total**. GL smoke is additional validation, not included in that count. The terrain comparison captures `worldPosition` from the real `terrain.vert` with transform feedback. CPU tests separately exercise interpolation inside each rendered triangle. The smoke test invokes the real game action handlers; it does not claim that Windows OS keyboard delivery or Russian-layout behaviour was tested on this Linux machine.

Mukhamejan's `HeightField`, `Player`, `Camera`, terrain/player/lit shaders and diagnostics are unchanged apart from moving into directories. Integration adapts Renderer/main/Shader/Keyboard and adds Game/Obstacles plus tests. Obstacles use centre-point ground contact and remain upright; full support over steep-slope corners and obstacle collision are not implemented.

## Reference frame

![Actual integrated screenshot: 8 metres, displaced ground, player and 25 submitted obstacles](images/stage1-integrated.png)

The pool has 96 entries; the renderer submits only entries whose whole Z extent lies on the terrain mesh. This avoids floating boxes beyond the far end of the ground. It is not full camera-frustum culling. The reference frame has 25 submitted obstacles and one draw in instanced mode. The HUD explicitly identifies collision/game-over as not yet implemented.

## Render timing sample

Measured on llvmpipe (LLVM 19.1.7, 256 bits), Mesa 25.0.7, cloud CPU. A fixed scene at one simulated second, 1280×720, 20 warmup frames followed by 120 measured frames per mode, `glFinish` after each render. No update, instance upload, HUD or buffer swap is timed. Frame-rate numbers are software-renderer wall timings, not hardware GPU queries.

| Mode | Pool | Submitted | Obstacle draws | Frame ms | FPS |
|---|---|---|---|---|---|
| ON | 96 | 25 | 1 | 7.798 | 128.243 |
| OFF | 96 | 25 | 25 | 8.182 | 122.214 |

This is one sample, not a claim of a universal speedup or proof of ≥30 FPS on the lab PC. A bigger `--instances` pool may still produce the same number of on-terrain instances. The old standalone timing table in `docs/performance.csv` belongs to the earlier prototype; do not mix the datasets.

## Reproduce

```sh
bash scripts/setup-cloud.sh
bash scripts/cloud-run.sh --smoke-test --screenshot artifacts/stage1-integrated.ppm
MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330 bash scripts/cloud-run.sh --smoke-test
bash scripts/cloud-run.sh --benchmark
```

Windows MSVC compilation, physical lab GPU performance, OS-level Windows input and the final video were not tested here. Obstacle collision/game-over and speed progression remain the next stage.
