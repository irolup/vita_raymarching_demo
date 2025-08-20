#version 120

#ifdef GL_ES
precision highp float;
#else
precision highp float;
#endif

varying vec2 vUV;

uniform vec2  uResolution;
uniform float uTime;
uniform vec3  uCamOrigin;
uniform vec3  uCamTarget;
uniform float uFov;
uniform vec3  uSunDir;

uniform float material_metallic;
uniform float material_roughness;
uniform float material_f0;
uniform float material_ambient;
uniform float material_brightness;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightIntensity;

float PI = 3.14159265;

// ---------- Utility ----------
float saturate_f(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate_v3(vec3 v) { return clamp(v, 0.0, 1.0); }

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 f = normalize(ta - ro);
    vec3 r = normalize(cross(vec3(0.0, 1.0, 0.0), f));
    vec3 u = cross(f, r);
    return mat3(r, u, f); 
}

//PBR functions:
float D_TrowbridgeReitz(vec3 n, vec3 h, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float ndh = max(dot(n,h),0.0);
    float ndh2 = ndh*ndh;
    float denom = (ndh2*(a2-1.0)+1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float G_Smith(vec3 n, vec3 l, vec3 v, float roughness){
    float k = (roughness+1.0)*(roughness+1.0)/8.0;
    float ndl = max(dot(n,l),0.0);
    float ndv = max(dot(n,v),0.0);
    return ndl/(ndl*(1.0-k)+k) * ndv/(ndv*(1.0-k)+k);
}

vec3 F_Schlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(1.0-cosTheta,5.0);
}

// ---------- SDFs ----------
float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdPlane(vec3 p, vec3 n, float h) {

    return dot(p, normalize(n)) + h;
}

int g_matID;

// Scene distance
float map(vec3 p) {
    float dPlane = sdPlane(p, vec3(0.0, 1.0, 0.0), 0.0);

    vec3 sp = p - vec3(0.0, 0.8, 0.0);
    sp.xz += vec2(sin(uTime * 0.5), cos(uTime * 0.4)) * 0.4;
    float dSphere = sdSphere(sp, 0.7);

    float d = dPlane;
    g_matID = 1;
    if (dSphere < d) { 
        d = dSphere; 
        g_matID = 2; 
    }

    vec3 sp2 = p - vec3(1.5, 0.4, 1.0);
    float dSphere2 = sdSphere(sp2, 0.35);
    if (dSphere2 < d) { 
        d = dSphere2; 
        g_matID = 3; 
    }

    return d;
}

vec3 calcNormal(vec3 p) {
    const vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = 0.0015;
    return normalize(
        e.xyy * map(p + e.xyy * eps) +
        e.yyx * map(p + e.yyx * eps) +
        e.yxy * map(p + e.yxy * eps) +
        e.xxx * map(p + e.xxx * eps)
    );
}

float softshadow(vec3 ro, vec3 rd, float k) {
    float res = 1.0;
    float t = 0.02;
    for (int i = 0; i < 16; i++) {
        float h = map(ro + rd * t);
        res = min(res, k * h / t);
        t += clamp(h, 0.02, 0.1);
        if (h < 0.0005) return 0.0;
        if (t > 10.0) break;
    }
    return saturate_f(res);
}

float ao(vec3 p, vec3 n) {
    float occ = 0.0;
    float sca = 1.0;
    for (int i = 1; i <= 3; i++) {
        float hr = 0.05 * float(i);
        float d = map(p + n * hr);
        occ += (hr - d) * sca;
        sca *= 0.6;
    }
    return saturate_f(1.0 - 1.5 * occ);
}

vec3 g_hitPos;
int g_hitMatID;

// ---------- Raymarch ----------
bool trace(vec3 ro, vec3 rd) {
    float t = 0.01;
    for (int i = 0; i < 100; i++) {
        vec3 currentPos = ro + rd * t;
        float h = map(currentPos);

        if (h < 0.01) {
            g_hitPos = currentPos;
            g_hitMatID = g_matID;
            return true;
        }

        t += max(h * 0.5, 0.01);
        if (t > 20.0) return false;
    }
    return false;
}

// ---------- Materials ----------
float checkerboard(vec3 p, float size) {
    vec3 c = floor(p / size);
    return mod(c.x + c.z, 2.0);
}

vec3 shade(int matID, vec3 p, vec3 n, vec3 ro, vec3 rd) {
    vec3 v = normalize(ro - p);

    // Albedo
    vec3 albedo;
    if(matID==1) {
        if(abs(p.y) < 0.1) {
            float checker = checkerboard(p, 1.0);
            vec3 color1 = vec3(0.9, 0.9, 0.9);
            vec3 color2 = vec3(0.2, 0.2, 0.2);
            albedo = mix(color1, color2, checker);
        } else {
            albedo = vec3(0.45, 0.47, 0.50);
        }
    }
    else if(matID==2) albedo=vec3(1.0,0.2,0.1);
    else if(matID==3) albedo=vec3(0.1,0.3,1.0);
    else albedo=vec3(0.7,0.7,0.7);

    // Metallic/roughness/F0
    float metallic = material_metallic;
    float roughness = material_roughness;
    vec3 baseF0 = vec3(material_f0, material_f0, material_f0);
    vec3 F0 = mix(baseF0, albedo, metallic);

    // Point light direction
    vec3 Lp = normalize(pointLightPos - p);
    float NdotLp = max(dot(n, Lp), 0.0);
    float dist = length(pointLightPos - p);
    float att = pointLightIntensity / (1.0 + dist * dist);
    
    att *= 10.0; 
    NdotLp = pow(NdotLp, 0.7);

    // Specular
    vec3 H = normalize(Lp + v);
    float D = D_TrowbridgeReitz(n,H,roughness);
    float G = G_Smith(n,Lp,v,roughness);
    vec3 F = F_Schlick(max(dot(H,v),0.0), F0);
    vec3 spec = F * D * G / (4.0 * max(dot(n,Lp),0.001) * max(dot(n,v),0.001));
    spec = clamp(spec * 1.5, 0.0, 1.0); // Reduced from 5.0 to 1.5

    // Diffuse + ambient
    float diff = NdotLp * att;
    float amb = material_ambient * ao(p,n);

    // Reflection/env
    vec3 r = reflect(-v, n);
    vec3 sky = vec3(0.55,0.65,0.95);
    vec3 ground = vec3(0.35,0.35,0.38);
    float t = 0.5 + 0.5*r.y;
    vec3 env = ground*(1.0-t) + sky*t;

    // Fresnel (already computed earlier as F)
    vec3 envReflection = F * env;  

    vec3 diffuseComponent = albedo * (amb + diff * pointLightColor * 7.0);
    vec3 specularComponent = spec * pointLightColor;

    // Final color with enhanced lighting
    vec3 color = diffuseComponent + specularComponent + envReflection;
    color *= material_brightness;

    return saturate_v3(color);
}

void main() {
    vec2 uv=vUV;
    vec2 p=uv*2.0-1.0;
    p.x*=(uResolution.x/uResolution.y);

    mat3 cam=lookAt(uCamOrigin,uCamTarget);
    float f=1.0/tan(uFov*0.5);
    vec3 rd=normalize(cam * vec3(p.x,p.y,f));
    vec3 ro=uCamOrigin;

    vec3 col;
    if(trace(ro, rd)){
        vec3 pos = g_hitPos;
        int matID = g_hitMatID;
        vec3 n = calcNormal(pos);
        col = shade(matID, pos, n, ro, rd);

        // Only reflect spheres (matID 2)
        if(matID == 2){
            vec3 refl = reflect(rd, n);
            if(trace(pos + n*0.01, refl)){
                vec3 pos2 = g_hitPos;
                int mat2 = g_hitMatID;
                vec3 n2 = calcNormal(pos2);
                vec3 c2 = shade(mat2, pos2, n2, ro, refl);
                col = mix(col, c2, 0.22);

                // second reflection
                vec3 refl2 = reflect(refl, n2);
                if(trace(pos2 + n2*0.001, refl2)){
                    vec3 pos3 = g_hitPos;
                    int mat3 = g_hitMatID;
                    vec3 n3 = calcNormal(pos3);
                    vec3 c3 = shade(mat3, pos3, n3, ro, refl2);
                    col = mix(col, c3, 0.12);
                }
            }
        }
    }
    else{
        float t=0.5+0.5*rd.y;
        col=vec3(0.35,0.36,0.40)*(1.0-t)+vec3(0.6,0.75,0.95)*t;
    }

    gl_FragColor = vec4(col,1.0);
}
