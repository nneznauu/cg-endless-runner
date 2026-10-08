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
    vec2 uv = vec2(xz.x / terrainWidth + 0.5,
                   (xz.y - scrollPhase) / terrainPeriod);
    return textureLod(heightMap, uv, 0.0).r * heightScale;
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
