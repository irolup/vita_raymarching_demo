// Uniforms for raymarching
uniform float2 uResolution;
uniform float uTime;
uniform float3 uCamOrigin;
uniform float3 uCamTarget;
uniform float uFov;
uniform float3 uSunDir;


uniform float material_metallic;
uniform float material_roughness;
uniform float material_f0;
uniform float material_ambient;
uniform float material_brightness;

uniform float3 pointLightPos;
uniform float3 pointLightColor;
uniform float pointLightIntensity;

float PI = 3.14159265;

// ---------- Utility ----------

float saturate_f(float x) { return clamp(x, 0.0f, 1.0f); }
float3 saturate_v3(float3 v) { return clamp(v, 0.0f, 1.0f); }

float3x3 lookAt(float3 ro, float3 ta) {
    float3 f = normalize(ta - ro);
    float3 r = normalize(cross(float3(0.0f, 1.0f, 0.0f), f));
    float3 u = cross(f, r);
    return float3x3(r, u, f);
}

//PBR functions:
float D_TrowbridgeReitz(float3 n, float3 h, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float ndh = max(dot(n,h),0);
    float ndh2 = ndh*ndh;
    float denom = (ndh2*(a2-1.0)+1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float G_Smith(float3 n, float3 l, float3 v, float roughness){
    float k = (roughness+1)*(roughness+1)/8.0;
    float ndl = max(dot(n,l),0);
    float ndv = max(dot(n,v),0);
    return ndl/(ndl*(1.0-k)+k) * ndv/(ndv*(1.0-k)+k);
}

float3 F_Schlick(float cosTheta, float3 F0){
    return F0 + (1.0 - F0) * pow(1.0-cosTheta,5.0);
}

// ---------- SDFs ----------
float sdSphere(float3 p, float r) {
    return length(p) - r;
}

float sdPlane(float3 p, float3 n, float h) {
    return dot(p, normalize(n)) + h;
}

// Scene distance function
float map(float3 p, out int matID) {
    float dPlane = sdPlane(p, float3(0,1,0), 0);

    float3 sp = p - float3(0,0.8,0);
    sp.x += sin(uTime * 0.5) * 0.4;
    sp.z += cos(uTime * 0.4) * 0.4;
    float dSphere = sdSphere(sp, 0.7);

    float d = dPlane;
    matID = 1; // ground
    if(dSphere < d) { d = dSphere; matID = 2; }

    float3 sp2 = p - float3(1.5,0.4,1.0);
    float dSphere2 = sdSphere(sp2, 0.35);
    if(dSphere2 < d) { d = dSphere2; matID = 3; }

    return d;
}

// Numerical normal
float3 calcNormal(float3 p) {
    float2 e = float2(1,-1)*0.5773;
    float eps = 0.0015;
    int junk;
    return normalize(
        e.xyy*map(p+e.xyy*eps, junk) +
        e.yyx*map(p+e.yyx*eps, junk) +
        e.yxy*map(p+e.yxy*eps, junk) +
        e.xxx*map(p+e.xxx*eps, junk)
    );
}

// Soft shadow (simplified)
float softshadow(float3 ro, float3 rd, float k) {
    float res=1;
    float t=0.02;
    int junk;
    for(int i=0;i<16;i++){
        float h=map(ro+rd*t, junk);
        res=min(res,k*h/t);
        t+=clamp(h,0.02,0.1);
        if(h<0.0005) return 0;
        if(t>10) break;
    }
    return saturate_f(res);
}

// Ambient occlusion (simplified)
float ao(float3 p, float3 n) {
    float occ=0; float sca=1;
    int junk;
    for(int i=1;i<=3;i++){
        float hr=0.05*i;
        float d=map(p+n*hr, junk);
        occ+=(hr-d)*sca;
        sca*=0.6;
    }
    return saturate_f(1.0-1.5*occ);
}

// ---------- Raymarch ----------
bool trace(float3 ro, float3 rd, inout float3 pos, inout int matID) {
    float t = 0.01f;
    for (int i = 0; i < 100; i++) {
        float3 currentPos = ro + rd * t;
        int mid;
        float h = map(currentPos, mid);

        if (h < 0.01f) {
            pos = currentPos;
            matID = mid;
            return true;
        }

        t += max(h * 0.5f, 0.01f);
        if (t > 20.0f) return false;
    }
    return false;
}

// ---------- Materials ----------

// Checkerboard pattern function
float checkerboard(float3 p, float size) {
    float3 c = floor(p / size);
    return fmod(c.x + c.z, 2.0);
}

float3 shade(int matID, float3 p, float3 n, float3 ro, float3 rd) {
    float3 v = normalize(ro - p);

    // Albedo
    float3 albedo;
    if(matID==1) {
        if(abs(p.y) < 0.1) {
            float checker = checkerboard(p, 1.0);
            float3 color1 = float3(0.9, 0.9, 0.9);
            float3 color2 = float3(0.2, 0.2, 0.2);
            albedo = lerp(color1, color2, checker);
        } else {
            albedo = float3(0.45, 0.47, 0.50);
        }
    }
    else if(matID==2) albedo=float3(1,0.2,0.1);
    else if(matID==3) albedo=float3(0.1,0.3,1);
    else albedo=float3(0.7,0.7,0.7);

    // Metallic/roughness/F0
    float metallic = material_metallic;
    float roughness = material_roughness;
    float3 baseF0 = float3(material_f0, material_f0, material_f0);
    float3 F0 = lerp(baseF0, albedo, metallic);

    // Point light direction
    float3 Lp = normalize(pointLightPos - p);
    float NdotLp = max(dot(n, Lp), 0.0);
    float dist = length(pointLightPos - p);
    float att = pointLightIntensity / (1.0 + dist * dist);
    

    // Specular
    float3 H = normalize(Lp + v);
    float D = D_TrowbridgeReitz(n,H,roughness);
    float G = G_Smith(n,Lp,v,roughness);
    float3 F = F_Schlick(max(dot(H,v),0), F0);
    float3 spec = F * D * G / (4.0 * max(dot(n,Lp),0.001) * max(dot(n,v),0.001));
    spec = clamp(spec * 1.5, 0.0, 1.0);

    // Diffuse + ambient
    float diff = NdotLp * att;
    float amb = material_ambient * ao(p,n);

    // Reflection/env
    float3 r = reflect(-v, n);
    float3 sky = float3(0.55,0.65,0.95);
    float3 ground = float3(0.35,0.35,0.38);
    float t = 0.5 + 0.5*r.y;
    float3 env = ground*(1-t) + sky*t;

    // Fresnel (already computed earlier as F)
    float3 envReflection = F * env;  

    float3 diffuseComponent = albedo * (amb + diff * pointLightColor);
    float3 specularComponent = spec * pointLightColor;

    // Final color
    float3 color = diffuseComponent + specularComponent + envReflection;
    color *= material_brightness;

    return saturate_v3(color);
}

float4 main(
    float2 vUV : TEXCOORD0
) {
    float2 uv=vUV;
    float2 p=uv*2-1;
    p.x*=(uResolution.x/uResolution.y);

    float3x3 cam=lookAt(uCamOrigin,uCamTarget);
    float f=1.0/tan(uFov*0.5);
    float3 rd=normalize(mul(float3(p.x,p.y,f),cam));
    float3 ro=uCamOrigin;

    float3 pos; int matID; float3 col;
    if(trace(ro, rd, pos, matID)){
        float3 n = calcNormal(pos);
        col = shade(matID, pos, n, ro, rd);

        // Only reflect spheres (matID 2)
        if(matID == 2){
            float3 refl = reflect(rd, n);
            float3 pos2; int mat2;
            if(trace(pos + n*0.01, refl, pos2, mat2)){
                float3 n2 = calcNormal(pos2);
                float3 c2 = shade(mat2, pos2, n2, ro, refl);
                col = lerp(col, c2, 0.22);

                // second reflection
                float3 refl2 = reflect(refl, n2);
                float3 pos3; int mat3;
                if(trace(pos2 + n2*0.001, refl2, pos3, mat3)){
                    float3 n3 = calcNormal(pos3);
                    float3 c3 = shade(mat3, pos3, n3, ro, refl2);
                    col = lerp(col, c3, 0.12);
                }
            }
        }
    }
    else{
        float t=0.5+0.5*rd.y;
        col=float3(0.35,0.36,0.40)*(1-t)+float3(0.6,0.75,0.95)*t;
    }

    return float4(col,1);
}