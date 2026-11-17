Terrain, player, camera and build
This section covers the height-map ground, lighting on the terrain, player movement, the camera, the Windows build and the instancing measurements.
Height-map ground
The ground is a 128 × 128 height texture made on the CPU from sine and cosine functions, so the terrain repeats every 128 metres along the road. The vertex shader moves the vertices of a fixed grid up or down using this texture:
h(x,z,s)=A H(x/24+0.5, (z-s)/128)
H is the height texture (interpolated bilinearly), s is the scroll phase shared by the whole game, and A is 2.5 when displacement is on and 0 when it is off. In the shader we read the four nearest texels with texelFetch and interpolate them ourselves with the same formula as in the CPU code. We did it this way because the built-in texture filter can give slightly different results on different GPUs. Across the road the texture is clamped, and along the road it repeats. The T key turns displacement on and off, and the player and obstacles follow the change.
The player stays near Z = 0 and the world moves towards it. Obstacles are recycled and the scroll phase wraps back into [0, 128), so coordinates stay small even in a long run. The distance travelled is stored separately as a double.
On the CPU, HeightField::surfaceHeight interpolates over the same triangles as the drawn mesh, so the player stands on the surface that is actually visible. An OpenGL validation test compares the vertices transformed by the shader with the CPU values for five combinations of phase and displacement.
Normals and lighting
For each vertex the shader reads four neighbouring heights 0.5 m away and computes the normal:
n=normalize(h_W-h_E, 2δ, h_B-h_F), δ=0.5
The vertex shader sends the world position, normal, base colour and ground coordinates to the fragment shader. There we draw the road markings and add ambient light, Lambert diffuse light and a Blinn–Phong highlight with exponent 28. The J and L keys rotate the light.
Fog mixes the colour towards the background colour with the factor
f(d)=1-exp(-0.017 d)
where d is the distance from the camera to the point.
The terrain goes through these steps: the grid and height texture are the input, the vertex shader displaces the vertices and estimates normals, the fragment shader does lighting and fog, and then the depth test is applied. Obstacles use their own vertex shader with a transform for each instance. The HUD is drawn last, on top of the scene.
Player and camera
A and D move the player sideways, and Space makes it jump when it is on the ground. The player's feet follow the height-map surface. In the air gravity changes the vertical speed, and on landing the player returns to the ground.
The default camera follows the player. The C key switches to an orbit camera: the arrow keys rotate it and the mouse wheel zooms. The projection uses the current window aspect ratio, so the picture is not stretched when the window is resized.
Reproducible Windows build
CMake builds the project on any machine. It generates the GLAD 1 loader, downloads GLM 1.0.1 and FreeGLUT 3.4.0 as source archives (checked against hashes) and builds FreeGLUT as a static library. No path depends on one person's computer.
The script scripts/build-windows.ps1 finds Visual Studio 2022, builds Release x64 and runs CTest. With extra arguments it can also run the OpenGL validation, the performance tests or the game itself. Shader files stay outside the executable and are copied next to it.
Performance measurements
We compared instancing on and off on Windows with Intel Arc Graphics at 1280 × 720. Both modes drew 1,000 obstacles. Each mode was run three times with 120 measured frames per run. The results are in Table 1.
Table 1.
Instancing ON and OFF, 1,000 obstacles, 1280 × 720, Intel Arc Graphics.
Instancing Submitted obstacles Obstacle draw calls Median render time, ms
ON 1,000 3 0.465
OFF 1,000 1,000 1.058
With instancing, the number of obstacle draw calls fell from 1,000 to 3, one for each obstacle type. The median render time dropped from 1.058 ms to 0.465 ms, about 56% less in this scene.
These times include rendering and waiting for the GPU to finish. They do not include game updates, instance-buffer uploads, HUD drawing and buffer swapping, so they are not the full frame time of the game.
The script scripts/measure-performance.ps1 saves the raw logs, each sample, the median table, renderer and driver information, the executable hash and the timing method. Its -Instances argument sets the size of the stress scene, up to 10,000 obstacles.
