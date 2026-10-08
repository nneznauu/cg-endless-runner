#version 330 core

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 localNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 objectColor;

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 surfaceColor;
out vec2 groundCoordinate;

void main() {
    vec4 position = model * vec4(localPosition, 1.0);
    worldPosition = position.xyz;
    worldNormal = transpose(inverse(mat3(model))) * localNormal;
    surfaceColor = objectColor;
    groundCoordinate = vec2(0.0);
    gl_Position = projection * view * position;
}
