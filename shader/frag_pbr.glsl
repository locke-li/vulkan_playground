#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformLight {
	vec4 lightPos;
	vec4 lightData;
    vec4 cameraPos;
} lighting;

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 normal;

layout(set = 0, binding = 2) uniform sampler texSampler;
layout(set = 1, binding = 1) uniform texture2D baseTex;
layout(set = 1, binding = 2) uniform texture2D metallicRoughnessTex;
layout(set = 1, binding = 3) uniform texture2D normalMap;

layout(location = 0) out vec4 outColor;

#define PI 3.14159269793

//Lo(p, Wo) = integral|Fr(p, Wi, Wo) * Li(p, Wi) * n * Wi * dWi
//Li(p, Wi) = intensity/fallout light strength
//n * Wi * dWi = for infinitly small Wi(a point): n * Vi
//Fr(p, Wi, Wo) = BRDF: kS * F_cook-torrence + kD * F_lambert
// where:
//  kS = metalness
//  kD = 1 - kS
//  F_lambert = color / PI
//  F_cook-torrence = DFG / (4 * (Wo * n) * (Wi * n))

//D = (microfacet) normal distribution
// Trowbridge-Reitz GGX: D(n, h, a) = a^2 / (PI * ((n*h)^2 * (a^2-1) + 1)^2)
// where:
//  h = (Wi + Wo) / 2, half vector(=normal)

//F = fresnel factor
// Fresnel-Schlick: F(h, v, F0) = F0 + (1-F0)*(1 - (h*v))^5
// where:
//  F0 = base reflectivity, pre-calculated from different materials
//  approximation: F0 = mix(vec(0.04), surfaceColor.rgb, metalness)

//G = (microfacet) geometry occulusion
// Schlick-GGX: G(n, v, k) = (n*v)/((n*v)*(1-k) + k)
// where:
//  v = view direction
//  k = roughness factor, k_direct = (a+1)^2/8, k_ibl = a^2/2;
// this function takes one direction, for both direction (Smith Method):
// G(n, v, l, k) = G(n, v, k) * G(n, l, k)
// where:
// l = light direction

struct PBR_Data {
    vec3 lightDir;
    vec3 viewDir;
    vec3 normal;
    float metalness;
    float roughness;
    vec4 diffuseColor;
};

float NDF_TRGGX(vec3 l, vec3 v, vec3 n, float a) {
    vec3 h = (l + v) / 2;
    float a2 = a * a;
    float denom = dot(n, h);
    denom = denom * denom * (a2 - 1) + 1;
    return a2 / (PI * pow(denom, 2));
}

vec3 Fresnel_Schlick(vec3 c, float a) {
    return mix(vec3(0.04), c, a);
}

float GeometryRoughness(float a) {
    return pow(a + 1, 2) / 8;
}

float Geometry_Schlick(vec3 n, vec3 v, float k) {
    float ndotv = dot(n, v);
    return ndotv/(ndotv * (1-k) + k);
}

float Geometry_Smith(vec3 n, vec3 l, vec3 v, float a) {
    float k = GeometryRoughness(a);
    return Geometry_Schlick(n, v, k) * Geometry_Schlick(n, l, k);
}

vec3 BRDF_Specular(PBR_Data data) {
    float D = NDF_TRGGX(data.lightDir, data.viewDir, data.normal, data.roughness);
    vec3 F = Fresnel_Schlick(data.diffuseColor.rgb, data.metalness);
    float G = Geometry_Smith(data.normal, data.lightDir, data.viewDir, data.roughness);

    return D * F * G / (4 * dot(data.lightDir, data.normal) * dot(data.viewDir, data.normal));
}

vec3 BRDF_Diffuse(PBR_Data data) {
    return data.diffuseColor.rgb;
}

float calculateSpecularPbr(float metallic, float roughness) {
    return metallic * (1 - roughness);
}

vec3 BRDF(PBR_Data data) {
    float specularComp = calculateSpecularPbr(data.metalness, data.roughness);
    float diffuseComp = 1.0 - specularComp;
    return specularComp * BRDF_Specular(data) + diffuseComp * BRDF_Diffuse(data);
    //return vec3(diffuseComp);
}

void main() {
    PBR_Data pbr_data;
    vec4 metallicRoughness = texture(sampler2D(metallicRoughnessTex, texSampler), texCoord);
    pbr_data.metalness = metallicRoughness.b;
    pbr_data.roughness = metallicRoughness.g;
    pbr_data.diffuseColor = texture(sampler2D(baseTex, texSampler), texCoord);
    pbr_data.lightDir = lighting.lightPos.xyz - position;
    pbr_data.viewDir = lighting.cameraPos.xyz - position;
    pbr_data.normal = normal;
    vec3 brdf = BRDF(pbr_data);
    outColor = vec4(brdf, pbr_data.diffuseColor.a);
}