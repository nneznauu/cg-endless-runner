#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in mat4 aInstanceModel;
uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform bool uInstanced;
out vec3 vWorld;
out vec3 vNormal;
out vec3 vLocal;
void main() {
    mat4 model = uInstanced ? aInstanceModel : uModel;
    vec4 world = model * vec4(aPosition, 1.0);
    vWorld = world.xyz;
    vNormal = transpose(inverse(mat3(model))) * aNormal;
    vLocal = aPosition;
    gl_Position = uViewProjection * world;
}
