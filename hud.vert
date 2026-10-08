#version 330 core

layout(location = 0) in vec2 pixelPosition;
uniform vec2 screenSize;

void main() {
    vec2 ndc = vec2(2.0 * pixelPosition.x / screenSize.x - 1.0,
                    1.0 - 2.0 * pixelPosition.y / screenSize.y);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
