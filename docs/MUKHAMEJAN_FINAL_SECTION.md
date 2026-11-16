# Terrain player camera and build

This is the technical section for Mukhamejan's part of the team report. Incorporate the final Windows measurements and screenshots before submission. The application contains AI-assisted code; describe the actual module ownership, review, changes and testing rather than claiming unaided authorship.

## Height map ground

The CPU creates a 128 by 128 single-channel height texture. The data uses periodic sine and cosine functions and repeats every 128 metres along the running direction. A fixed triangle grid is displaced in the vertex shader. The implemented sampling equation is:

\[
h(x,z,s)=A H\left(\frac{x}{24}+0.5,\frac{z-s}{128}\right).
\]

Here H is the bilinearly interpolated height texture, s is the shared scroll phase, and A is 2.5 when displacement is enabled or 0 when disabled. The shader fetches four texels with `texelFetch` and interpolates them explicitly using the same formula as the CPU. This avoids depending on the hardware texture filter's weight precision. T switches displacement and updates player and obstacle ground placement together. Sampling clamps across the track and repeats along it.

The game keeps the player close to Z = 0, recycles obstacles and wraps s into the interval [0,128). Render coordinates therefore stay bounded during long runs. The distance score is stored separately and is not used as an unbounded float coordinate.

The CPU uses `HeightField::surfaceHeight` to interpolate the same two triangles as the rendered mesh. This avoids placing the player on a different mathematical surface from the one visible on screen. The OpenGL validation reads transformed positions from the actual terrain vertex shader and compares them with CPU sampling at five phase/displacement cases.

## Normals and lighting

The terrain shader samples four neighbouring heights with delta = 0.5 metres. Its normal is:

\[
\mathbf n=\operatorname{normalize}(h_W-h_E,2\delta,h_B-h_F).
\]

The vertex shader supplies world position, normal, material colour and scrolling ground coordinates to the fragment shader. The fragment shader generates the road markings and combines an ambient term, Lambert diffuse lighting and a Blinn–Phong highlight. The exponent is 28. J/L rotates the light direction. Distance fog blends towards the background colour using `1 - exp(-distance * 0.017)`. These operations use the project's own shader files, not a full game engine.

The shader pipeline is: grid and height texture; terrain vertex displacement and normal estimation; triangle rasterisation and interpolated attributes; fragment lighting and fog; depth-tested scene and HUD. The report's diagram should also show Daniyar's separate instanced-obstacle vertex path feeding its fragment shader.

## Player and camera

The player moves horizontally with A/D and jumps with Space while grounded. Its feet are placed on the height-map mesh. Gravity and vertical velocity govern airborne movement, and landing restores ground contact. The follow camera provides the gameplay view; C switches to orbit controls, with arrows for rotation and the wheel for zoom. Perspective projection uses the current window aspect ratio, so resizing preserves the scene's proportions.

## Reproducible build and measurements

The final CMake path includes generated GLAD 1 loader files, downloads hash-verified source archives of GLM 1.0.1 and FreeGLUT 3.4.0, and builds FreeGLUT statically. It removes the dependency on library folders under a particular student's Desktop or Downloads directory. `scripts/build-windows.ps1` locates Visual Studio 2022, builds Release x64, runs CTest, and optionally runs the existing OpenGL validation and performance measurements.

`scripts/measure-performance.ps1` records three ON/OFF repetitions of the fixed stress scene with 1000 submitted obstacles by default. The `-Instances` argument selects another count up to 10000. It saves raw logs, every sample, a median-time table, the active renderer and driver information, the executable hash, and the timing method. The benchmark measures rendering plus completion wait, excluding updates, uploads, HUD and buffer swapping. Normal gameplay FPS on the target lab computer must be reported separately.

## Evidence to insert after the final Windows run

Use screenshots from the final integrated build: a follow-camera gameplay frame and the same paused orbit view with height-map displacement ON/OFF. Give every figure a numbered caption and refer to it from the text. Add the actual table generated in `report-table.md`, identify the GPU and resolution, and include the demo-video link with accessible permissions. Do not reuse the historical standalone `docs/performance.csv` as final measurements.
