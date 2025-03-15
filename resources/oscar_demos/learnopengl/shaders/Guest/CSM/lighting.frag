#version 330 core

struct BaseLight {
    vec4 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight {
    BaseLight Base;
    vec3 Direction;
};

struct Attenuation {
    float Constant;
    float Linear;
    float Exp;
};

struct PointLight {
    BaseLight Base;
    vec3 Position;
    Attenuation Atten;
};

struct SpotLight {
    PointLight Base;
    vec3 Direction;
    float Cutoff;
};

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;
const int NUM_CASCADES = 3;

uniform int gNumPointLights;
uniform int gNumSpotLights;
uniform DirectionalLight gDirectionalLight;
uniform PointLight gPointLights[MAX_POINT_LIGHTS];
uniform SpotLight gSpotLights[MAX_SPOT_LIGHTS];
uniform vec4 gObjectColor;
uniform sampler2D gShadowMap[NUM_CASCADES];
uniform vec3 gEyeWorldPos;
uniform float gMatSpecularIntensity;
uniform float gSpecularPower;
uniform float gCascadeEndClipSpace[NUM_CASCADES];

in vec4 LightSpacePos[NUM_CASCADES];
in float ClipSpacePosZ;
in vec3 Normal0;
in vec3 WorldPos0;

out vec4 FragColor;

float CalcShadowFactor(int CascadeIndex, vec4 LightSpacePos)
{
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords = 0.5*ProjCoords.xy + 0.5;
    float z = 0.5 * ProjCoords.z + 0.5;
    float Depth = texture(gShadowMap[CascadeIndex], UVCoords).x;

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = Normal0;
    vec3 lightDir = gDirectionalLight.Direction;
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.00001);

    return z - bias > Depth ? 0.0 : 1.0;
}

vec4 CalcLightInternal(
    BaseLight Light,
    vec3 LightDirection,
    vec3 Normal,
    float ShadowFactor)
{
    vec4 AmbientColor = Light.Color * Light.AmbientIntensity;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor  = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        DiffuseColor = Light.Color * Light.DiffuseIntensity * DiffuseFactor;

        vec3 VertexToEye = normalize(gEyeWorldPos - WorldPos0);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gSpecularPower);
            SpecularColor = Light.Color * gMatSpecularIntensity * SpecularFactor;
        }
    }

    return (AmbientColor + ShadowFactor * (DiffuseColor + SpecularColor));
}

vec4 CalcDirectionalLight(vec3 Normal, float ShadowFactor)
{
    return CalcLightInternal(gDirectionalLight.Base, gDirectionalLight.Direction, Normal, ShadowFactor);
}

vec4 CalcPointLight(PointLight l, vec3 Normal)
{
    vec3 LightDirection = WorldPos0 - l.Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 Color = CalcLightInternal(l.Base, LightDirection, Normal, 1.0);
    float AttenuationFactor =  l.Atten.Constant +
                         l.Atten.Linear * Distance +
                         l.Atten.Exp * Distance * Distance;

    return Color / AttenuationFactor;
}

vec4 CalcSpotLight(SpotLight l, vec3 Normal)
{
    vec3 LightToPixel = normalize(WorldPos0 - l.Base.Position);
    float SpotFactor = dot(LightToPixel, l.Direction);

    if (SpotFactor > l.Cutoff) {
        vec4 Color = CalcPointLight(l.Base, Normal);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - l.Cutoff));
    }
    else {
        return vec4(0,0,0,0);
    }
}

void main()
{
    vec3 Normal = normalize(Normal0);

    float ShadowFactor = 0.0;
    vec4 CascadeIndicator = vec4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0 ; i < NUM_CASCADES ; i++) {
        if (ClipSpacePosZ <= gCascadeEndClipSpace[i]) {
            ShadowFactor = CalcShadowFactor(i, LightSpacePos[i]);

            if (i == 0) 
                CascadeIndicator = vec4(0.1, 0.0, 0.0, 0.0);
            else if (i == 1)
                CascadeIndicator = vec4(0.0, 0.1, 0.0, 0.0);
            else if (i == 2)
                CascadeIndicator = vec4(0.0, 0.0, 0.1, 0.0);

            break;
        }
   }

    vec4 TotalLight = CalcDirectionalLight(Normal, ShadowFactor);

    for (int i = 0 ; i < gNumPointLights ; i++) {
        TotalLight += CalcPointLight(gPointLights[i], Normal);
    }

    for (int i = 0 ; i < gNumSpotLights ; i++) {
        TotalLight += CalcSpotLight(gSpotLights[i], Normal);
    }

    vec4 SampledColor = gObjectColor;
    FragColor = SampledColor * TotalLight + 0.1*CascadeIndicator;
}
