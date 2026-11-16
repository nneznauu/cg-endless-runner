#version 330 core

layout(location = 0) in vec2 groundXZ;

uniform mat4 view;
uniform mat4 projection;
uniform sampler2D heightMap;
uniform float heightScale;
uniform float terrainWidth;
uniform float terrainPeriod;
uniform float scrollPhase;
uniform float normalStep;

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 surfaceColor;
out vec2 groundCoordinate;

float heightAt(vec2 xz) {
    ivec2 size = textureSize(heightMap, 0);
    vec2 uv = vec2(clamp(xz.x / terrainWidth + 0.5, 0.0, 1.0),
                   fract((xz.y - scrollPhase) / terrainPeriod));
    vec2 position = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 weight = fract(position);

    int x0 = clamp(base.x, 0, size.x - 1);
    int x1 = clamp(base.x + 1, 0, size.x - 1);
    int z0 = (base.y + size.y) % size.y;
    int z1 = (base.y + 1 + size.y) % size.y;

    float a = texelFetch(heightMap, ivec2(x0, z0), 0).r;
    float b = texelFetch(heightMap, ivec2(x1, z0), 0).r;
    float c = texelFetch(heightMap, ivec2(x0, z1), 0).r;
    float d = texelFetch(heightMap, ivec2(x1, z1), 0).r;
    float lower = a + (b - a) * weight.x;
    float upper = c + (d - c) * weight.x;
    return heightScale * (lower + (upper - lower) * weight.y);
}

void main() {
    float y = heightAt(groundXZ);
    float west = heightAt(groundXZ - vec2(normalStep, 0.0));
    float east = heightAt(groundXZ + vec2(normalStep, 0.0));
    float back = heightAt(groundXZ - vec2(0.0, normalStep));
    float front = heightAt(groundXZ + vec2(0.0, normalStep));

    worldPosition = vec3(groundXZ.x, y, groundXZ.y);
    worldNormal = normalize(vec3(west - east, 2.0 * normalStep, back - front));
    groundCoordinate = vec2(groundXZ.x, groundXZ.y - scrollPhase);
    float shoulder = smoothstep(4.6, 5.4, abs(groundXZ.x));
    vec3 grass = mix(vec3(0.11, 0.29, 0.23), vec3(0.27, 0.43, 0.28),
                     clamp(y / 2.5, 0.0, 1.0));
    surfaceColor = mix(vec3(0.17, 0.23, 0.27), grass, shoulder);
    gl_Position = projection * view * vec4(worldPosition, 1.0);
}
