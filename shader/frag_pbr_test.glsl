#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformLight {
    vec4 debugOption;
    vec4 cameraPos;
	vec4 lightPos;
	vec4 lightData;
} lighting;

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec4 outColor;

#define PI 3.141592653589793
#define NEAR_ZERO 1e-4

//Lo(p, Wo) = integral|Fr(p, Wi, Wo) * Li(p, Wi) * n * Wi * dWi
//Li(p, Wi) = intensity/fallout light strength
//n * Wi * dWi = for infinitly small Wi(a point): n * Vi
//Fr(p, Wi, Wo) = BRDF: kS * F_cook-torrence + kD * F_lambert
// where:
//  kS = F
//  kD = (1 - kS) * (1 - metalness)
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

float NDF_TRGGX(float ndoth, float a) {
    float a2 = a * a;
    float denom = ndoth * ndoth * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

vec3 Fresnel_Schlick(vec3 c, float m, float vdoth) {
    vec3 baseReflectivity = mix(vec3(0.04), c, m);
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - vdoth, 5.0);
}

float GeometryRoughness(float a) {
	//return max(a * a / 2.0, NEAR_ZERO);
    return max(pow(a + 1.0, 2.0) / 8.0, NEAR_ZERO);
}

float Geometry_Schlick(float ndotw, float k) {
    return ndotw / (ndotw * (1.0-k) + k);
}

float Geometry_Smith(float ndotl, float ndotv, float a) {
    float k = GeometryRoughness(a);
    return Geometry_Schlick(ndotv, k) * Geometry_Schlick(ndotl, k);
}

vec3 BRDF_Specular(PBR_Data data, float ndotl, out vec3 kS) {
    vec3 h = normalize(data.lightDir + data.viewDir);
    float ndoth = max(dot(data.normal, h), 0.0);
	float vdoth = max(dot(data.viewDir, h), 0.0);
	float ndotv = max(dot(data.normal, data.viewDir), 0.0);
	float roughness = data.roughness * data.roughness;
    float D = NDF_TRGGX(ndoth, roughness);
    vec3 F = Fresnel_Schlick(data.diffuseColor.rgb, data.metalness, vdoth);
	kS = F;
    float G = Geometry_Smith(ndotl, ndotv, roughness);
    if(lighting.debugOption.x == 1.0) {
	    return vec3(D, F.r, G);
    }
    if(lighting.debugOption.x == 2.0) {
        return vec3(max(4.0 * ndotl * ndotv, NEAR_ZERO), D*F.r*G, 0);
    }
    if(lighting.debugOption.x == 3.0) {
        return vec3(ndotl, ndotv, 0);
    }
    if(lighting.debugOption.x == 4.0) {
	    return vec3(1/ndotl, 1/ndotv, 0.0);
    }
    return D * F * G / max(4.0 * ndotl * ndotv, NEAR_ZERO);
}

vec3 BRDF_Diffuse(PBR_Data data) {
    return data.diffuseColor.rgb / PI;
}

vec3 BRDF(PBR_Data data) {
	float ndotl = max(dot(data.normal, data.lightDir), 0.0);
	vec3 kS;
    vec3 specular = BRDF_Specular(data, ndotl, kS);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - data.metalness);
    return (specular + kD * BRDF_Diffuse(data)) * ndotl;
}

void main() {
    PBR_Data pbr_data;
    pbr_data.metalness = lighting.debugOption.y;
    pbr_data.roughness = lighting.debugOption.z;
    pbr_data.diffuseColor = vec4(1.0,1.0,1.0,1.0);
    pbr_data.lightDir = normalize(lighting.lightPos.xyz - position);
    pbr_data.viewDir = normalize(lighting.cameraPos.xyz - position);
    pbr_data.normal = normalize(normal);
    vec3 brdf = pow(BRDF(pbr_data), vec3(2.2));
    outColor = vec4(brdf, pbr_data.diffuseColor.a);
}