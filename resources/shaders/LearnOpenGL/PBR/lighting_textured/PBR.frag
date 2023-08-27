#version 330 core

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;
uniform vec3 uLightWorldPositions[4];
uniform vec3 uLightRadiances[4];
uniform vec3 uCameraWorldPosition;

in vec2 TexCoord;
in vec3 WorldPos;
in vec3 Normal;

out vec4 FragColor;

const float PI = 3.14159265359;

// see: https://www.jordanstevenstechart.com/physically-based-rendering
//
// it's a very good resource that explains each part of the rendering equation
// for simple PBR shaders

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal
// mapping the usual way for performance anyways; I do plan make a note of this
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(uNormalMap, TexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoord);
    vec2 st2 = dFdy(TexCoord);

    vec3 N   =  normalize(Normal);
    vec3 T   =  normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float TrowbridgeReitzGGXNDF(vec3 N, vec3 H, float roughness)
{
    // normal distribution function (NDF): "Trowbridge-Reitz GGX"
    //
    // this calculates the relative surface area of microfacets on a surface
    // with normal N that are exactly along the halfway reflection vector H.

    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float SchlickGGXGSF(float NdotV, float roughness)
{
    // geometric shadowing function (GSF): "Schlick-GGX"

    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float SmithWalterEtAlGSF(vec3 N, vec3 V, vec3 L, float roughness)
{
    // geometric shadowing function (GSF): Smith-based
    //
    // combines a shadowing component from the light ray (L) being occluded by
    // the surface roughness and a obstruction component from the view vector
    // being occluded by the surface roughness
    //
    // see: https://www.jordanstevenstechart.com/physically-based-rendering
    //
    // LearnOpenGL uses the "common form" of Smith-based GGX GSF, created by Walter
    // et. al. ("Microfacet Models for Refraction through Rough Surfaces", 2007)

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float geomObstruction = SchlickGGXGSF(NdotV, roughness);
    float geomShadowing = SchlickGGXGSF(NdotL, roughness);

    return geomObstruction * geomShadowing;
}

vec3 SchlickFresnelFunction(float cosTheta, vec3 F0)
{
    // fresnel function: Schlick Fresnel

    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // read material properties from material map textures
    vec3 albedo = texture(uAlbedoMap, TexCoord).rgb;
    float metallic = texture(uMetallicMap, TexCoord).r;
    float roughness = texture(uRoughnessMap, TexCoord).r;
    float ao = texture(uAOMap, TexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(uCameraWorldPosition - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int lightIdx = 0; lightIdx < 4; ++lightIdx)
    {
        // calculate per-light radiance
        vec3 L = normalize(uLightWorldPositions[lightIdx] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(uLightWorldPositions[lightIdx] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = uLightRadiances[lightIdx] * attenuation;

        // Cook-Torrance BRDF
        float NDF = TrowbridgeReitzGGXNDF(N, H, roughness);
        float G   = SmithWalterEtAlGSF(N, V, L, roughness);
        vec3 F    = SchlickFresnelFunction(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;

        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;

        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        vec3 fr = (kD * albedo / PI) + specular;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

        // add to outgoing radiance Lo
        Lo += fr * radiance * NdotL;
    }

    // add ambient component
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));  // HDR tonemap

    FragColor = vec4(color, 1.0);
}
