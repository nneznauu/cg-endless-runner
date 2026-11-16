# Integration contract

Base: teammate commit `b461826`. All application source lives in `src/` and all shaders in `shaders/`. `EndlessRunner.vcxproj` and CMake compile the same application modules. The existing Windows `Dependencies.props` paths remain valid. FreeGLUT creates the only window; GLAD 1 loads the only OpenGL API. The old GLEW/GLFW/Hud wrapper is removed from the application.

```mermaid
flowchart LR
  Input[FreeGLUT / Keyboard] --> Main[main: simulate and key actions]
  Main --> Game[Game: clock, phase, distance, obstacles]
  Main --> Player[Player: movement and jump]
  Height[HeightField: CPU height texture] --> Player
  Height --> Placement[Renderer: updateObstacles]
  Game --> Placement
  Placement --> Instances[Obstacles: mat4 instance buffer]
  Height --> Terrain[Terrain texture and vertex shader]
  Instances --> Draw[Instanced draw / individual fallback]
  Terrain --> Scene[Shared depth, camera, lighting, HUD]
  Draw --> Scene
  Player --> Scene
```

## Coordinates, time and ground contact

- Right-handed world, +Y up, +X right, −Z forward; player Z = 0, obstacles approach along +Z.
- `Game` owns distance, best score, run state, obstacle positions and bounded phase. Speed grows linearly with running time from **8 m/s** by 0.2 m/s² and is capped at **22 m/s** (reached after 70 s). Distance is the closed-form integral of that speed over the elapsed running time, so 30 FPS and 144 FPS give the same distance and speed. Each update derives one `travel` value that moves the obstacles and advances `groundPhase`. `Game::speed()` is the current world speed and is 0 outside Running; `currentSpeed()` is the run speed shown in the HUD. `Game::GroundPeriod` references `HeightField::period`, currently **128 m**; there is no separate 256 m runtime constant.
- `main::simulate` updates Game once, copies the current phase to renderer/player arguments, updates Player, then calls `Game::checkCollision`. Ready, Paused and GameOver prevent all of these updates. The interactive dt cap is the teammate's 0.05 s.
- Height texture coordinates use `(z - phase) / period`. `Renderer::updateObstacles` calls `terrain.surfaceHeight(x, z, game.groundPhase(), displaced)` with **local z**, not `phase-z` or a second phase subtraction.
- `surfaceHeight` matches texture filtering and the triangles used by the terrain index buffer. The obstacle bottom centre uses this height; centre Y adds half its height. Boxes remain upright; steep-slope corner support is not implemented.
- Player has full dimensions 0.8 × 1.2 × 0.8. `feetY` is the bottom and horizontal motion is clamped to [−4.2,4.2].
- Rendering filters obstacles whose full Z extent would exceed the terrain mesh. Off-terrain pool entries continue moving/recycling but cannot float beyond the end of the mesh. This is terrain-range filtering, not full camera-frustum culling.

## Run states and collision

| State | World | Keys |
|---|---|---|
| Ready | Program start. World, obstacles, score and player stand still; a centred banner asks to start. | Space or Enter starts; R resets and stays Ready |
| Running | World, obstacles, score and speed advance; collisions are checked every update. | Space jumps; P pauses; R starts a fresh run |
| Paused | Everything frozen; jump is blocked. | P resumes; R resets and stays Paused |
| GameOver | Entered on collision. World, obstacles, score, phase and player are frozen; jump and pause are disabled; banner shows score, best and restart hint. | Enter or R starts a new run immediately |

A restart resets the player, obstacle pool, distance, phase, elapsed time and speed. The best distance is kept for the session and is updated on game over and on reset.

`Game::collides(x, feetY, height)` takes the player position and a `GroundHeight` callback (`std::function<float(float,float)>`). The player box is 0.8 × `Player::height` × 0.8 at Z = 0, matching the scaled cube in `Renderer::draw`. Each obstacle box uses the same placement as `Obstacles::upload`: bottom centre at `height(x, z)`, top at that height plus the obstacle height. A collision requires strict overlap on X, Y and Z, so a jump whose feet stay above the obstacle top does not collide. Only obstacles that pass `Game::onTerrain`, the same farZ/nearZ filter used by `Renderer::updateObstacles`, are tested. `main` passes `HeightField::surfaceHeight` with the current phase and displacement flag, so collisions follow the visible terrain. Game has no OpenGL dependency.

At 22 m/s and the 0.05 s interactive dt cap an obstacle moves 1.1 m per step, less than the 2.15 m combined depth of the two boxes, so it cannot pass through the player between updates.

## Shared input and resources

P freezes distance/phase, player physics and obstacles together and works only from Running. R resets Game and Player and preserves Ready or Paused; from Running or GameOver it starts a fresh run. Enter starts from Ready and restarts from GameOver. Windows passes Enter (13) through the GLUT character callback together with Space and Esc; letters stay on virtual-key polling. T changes both the terrain VS height scale and the CPU ground sampler used by Player and obstacles. I switches draw submission without moving or hiding the obstacles. C retains follow/orbit; it is no longer the old standalone reset-view binding. Space remains jump. Windows virtual-key polling now includes I.

Renderer owns the obstacle program and resources. `Obstacles::initialize()` runs only after a GL context exists; `shutdown()` runs before it is destroyed. The global Renderer constructor performs no GL calls. Obstacles uses the shared `loadProgram`, GLAD and GLuint program API. All shaders stay external. The HUD shows the topic, team, run state, score, best, speed, FPS, technique toggles, visible obstacles and obstacle draw calls, plus the key list toggled by H. Ready, Paused and GameOver also draw a large centred banner.

`loadProgram` has an optional transform-feedback varying used to validate positions from the actual terrain vertex shader. Ordinary rendering is unchanged by this link-time option. The smoke test reads all mesh vertices and compares them to the CPU height sampler at several phases and with displacement disabled.

## Ownership

HeightField/Player/Camera and their shaders come from Mukhamejan's Stage 1 commit; Game/Obstacles and obstacle shaders are Daniyar's module. Main, Renderer, Shader, Keyboard and build files now contain integration changes. Source moves are recorded as Git renames; verified pre-integration backups are outside the checkout under `/workspace/review-backups/`.
