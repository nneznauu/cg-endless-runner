#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec3 surfaceColor;
in vec2 groundCoordinate;

uniform vec3 cameraPosition;
uniform vec3 directionToLight;
uniform bool groundPass;

out vec4 finalColor;

void main() {
    vec3 material = surfaceColor;
    if (groundPass) {
        float stripeDistance = abs(abs(groundCoordinate.x) - 1.6);
        float stripe = 1.0 - smoothstep(0.04, 0.11, stripeDistance);
        float dash = 1.0 - step(0.45, fract(groundCoordinate.y / 8.0));
        float edge = 1.0 - smoothstep(0.035, 0.085,
                                     abs(abs(groundCoordinate.x) - 4.55));
        material = mix(material, vec3(0.85, 0.69, 0.37),
                       max(stripe * dash, edge) * 0.75);
    }

    vec3 normal = normalize(worldNormal);
    vec3 light = normalize(directionToLight);
    vec3 toCamera = normalize(cameraPosition - worldPosition);
    float diffuse = max(dot(normal, light), 0.0);
    vec3 halfVector = normalize(light + toCamera);
    float highlight = pow(max(dot(normal, halfVector), 0.0), 28.0)
                      * step(0.0001, diffuse) * 0.12;
    vec3 shaded = material * (0.24 + 0.76 * diffuse)
                  + vec3(1.0, 0.89, 0.72) * highlight;

    float fog = 1.0 - exp(-length(cameraPosition - worldPosition) * 0.017);
    finalColor = vec4(mix(shaded, vec3(0.055, 0.085, 0.14), fog), 1.0);
}
