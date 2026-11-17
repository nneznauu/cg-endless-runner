#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec3 vLocal;
in vec3 vColor;
in vec3 vScale;
in vec3 vLocalNormal;
uniform vec3 uEye;
uniform vec3 uLightDirection;
uniform int uType;
uniform int uPatterns;
out vec4 fragColor;
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453);
}
vec3 logColor(vec3 p, vec3 a) {
    vec3 wood = vec3(0.72,0.52,0.30);
    if (a.x > a.y && a.x > a.z) return mix(wood, wood*0.55, step(0.75, fract(length(p.yz)*12.0)));
    return mix(wood, wood*0.65, step(0.8, fract((p.y+p.z)*14.0)));
}
vec3 barrierColor(vec3 p, vec3 a, vec3 size) {
    vec3 concrete = vec3(0.72,0.72,0.70);
    if (a.y > a.x && a.y > a.z) return concrete;
    if (p.y + size.y*0.5 < size.y*0.55) return concrete;
    float stripe = step(0.5, fract((p.x+p.y+p.z)*2.0));
    return mix(vec3(0.98,0.80,0.20), vec3(0.08,0.08,0.08), stripe);
}
vec3 brickColor(vec3 p, vec3 a, vec3 size) {
    vec3 brick = vec3(0.62,0.22,0.16);
    if (a.y > a.x && a.y > a.z) return brick;
    vec3 c = p + size*0.5;
    vec2 q = vec2(a.x > a.z ? c.z : c.x, c.y) / vec2(0.5,0.2);
    q.x += 0.5*mod(floor(q.y), 2.0);
    vec2 edge = min(fract(q), 1.0-fract(q)) * vec2(0.5,0.2);
    float mortar = 1.0 - step(0.015, min(edge.x, edge.y));
    vec3 tinted = brick*(0.88+0.24*hash(floor(q)));
    return mix(tinted, vec3(0.78,0.74,0.68), mortar);
}
void main() {
    vec3 base;
    if (uPatterns == 0) {
        float band = 1.0 - smoothstep(0.045, 0.065, abs(vLocal.y - 0.15));
        base = mix(vColor, vec3(0.98,0.84,0.61), band);
    } else {
        vec3 p = vLocal*vScale;
        vec3 a = abs(vLocalNormal);
        if (uType == 0) base = logColor(p, a);
        else if (uType == 1) base = barrierColor(p, a, vScale);
        else base = brickColor(p, a, vScale);
    }
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
