#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec3 vLocal;
uniform vec3 uEye;
uniform vec3 uLightDirection;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    float band = 1.0 - smoothstep(0.045, 0.065, abs(vLocal.y - 0.15));
    vec3 base = mix(uColor, vec3(0.98,0.84,0.61), band);
    vec3 n = normalize(vNormal);
    vec3 l = normalize(uLightDirection);
    vec3 v = normalize(uEye-vWorld);
    vec3 h = normalize(l+v);
    float diffuse = max(dot(n,l),0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n,h),0.0),28.0)*0.12 : 0.0;
    vec3 lit = base*(0.24+0.76*diffuse)+vec3(1.0,0.89,0.72)*specular;
    float fog = 1.0-exp(-length(uEye-vWorld)*0.017);
    fragColor = vec4(mix(lit,vec3(0.055,0.085,0.14),fog),1.0);
}
