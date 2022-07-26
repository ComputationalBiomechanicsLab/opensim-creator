#version 330 core

#define NR_POINT_LIGHTS 4

uniform vec3 uViewPos;

uniform vec3 uDirLightDirection;
uniform vec3 uDirLightAmbient;
uniform vec3 uDirLightDiffuse;
uniform vec3 uDirLightSpecular;

uniform vec3 uPointLightPos[NR_POINT_LIGHTS];
uniform float uPointLightConstant[NR_POINT_LIGHTS];
uniform float uPointLightLinear[NR_POINT_LIGHTS];
uniform float uPointLightQuadratic[NR_POINT_LIGHTS];
uniform vec3 uPointLightAmbient[NR_POINT_LIGHTS];
uniform vec3 uPointLightDiffuse[NR_POINT_LIGHTS];
uniform vec3 uPointLightSpecular[NR_POINT_LIGHTS];

uniform vec3 uSpotLightPosition;
uniform vec3 uSpotLightDirection;
uniform float uSpotLightCutoff;
uniform float uSpotLightOuterCutoff;
uniform float uSpotLightConstant;
uniform float uSpotLightLinear;
uniform float uSpotLightQuadratic;
uniform vec3 uSpotLightAmbient;
uniform vec3 uSpotLightDiffuse;
uniform vec3 uSpotLightSpecular;

uniform sampler2D uMaterialDiffuse;
uniform sampler2D uMaterialSpecular;
uniform float uMaterialShininess;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;


// calculates the color when using a directional light
vec3 CalcDirLight(
    vec3 dirDirection,
    vec3 dirAmbient,
    vec3 dirDiffuse,
    vec3 dirSpecular,
    vec3 normal,
    vec3 viewDir)
{
    vec3 lightDir = normalize(-dirDirection);

    // diffuse shading
    float diffuseStrength = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularStrength = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);

    // combine results
    vec3 ambientAmt = dirAmbient * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 diffuseAmt = dirDiffuse * diffuseStrength * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 specularAmt = dirSpecular * specularStrength * vec3(texture(uMaterialSpecular, TexCoords));

    return ambientAmt + diffuseAmt + specularAmt;
}

// calculates the color when using a point light
vec3 CalcPointLight(
    vec3 pointPos,
    float pointConstant,
    float pointLinear,
    float pointQuadratic,
    vec3 pointAmbient,
    vec3 pointDiffuse,
    vec3 pointSpecular,
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir)
{
    vec3 lightDir = normalize(pointPos - fragPos);

    // diffuse shading
    float diffuseStrength = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularStrength = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);

    // attenuation
    float distance = length(pointPos - fragPos);
    float attenuation = 1.0 / (pointConstant + pointLinear * distance + pointQuadratic * (distance * distance));    

    // combine results
    vec3 ambientAmt = pointAmbient * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 diffuseAmt = pointDiffuse * diffuseStrength * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 specularAmt = pointSpecular * specularStrength * vec3(texture(uMaterialSpecular, TexCoords));

    ambientAmt *= attenuation;
    diffuseAmt *= attenuation;
    specularAmt *= attenuation;

    return ambientAmt + diffuseAmt + specularAmt;
}

// calculates the color when using a spot light
vec3 CalcSpotLight(
    vec3 spotPosition,
    vec3 spotDirection,
    float spotCutoff,
    float spotOuterCutoff,
    float spotConstant,
    float spotLinear,
    float spotQuadratic,
    vec3 spotAmbient,
    vec3 spotDiffuse,
    vec3 spotSpecular,
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir)
{
    vec3 lightDir = normalize(spotPosition - fragPos);

    // diffuse shading
    float diffuseStrength = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-spotDirection, normal);
    float specularStrength = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);

    // attenuation
    float distance = length(spotPosition - fragPos);
    float attenuation = 1.0 / (spotConstant + spotLinear * distance + spotQuadratic * (distance * distance));

    // spotlight intensity
    float theta = dot(lightDir, normalize(-spotDirection));
    float epsilon = spotCutoff - spotOuterCutoff;
    float intensity = clamp((theta - spotOuterCutoff) / epsilon, 0.0, 1.0);

    // combine results
    vec3 ambientAmt = spotAmbient * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 diffuseAmt = spotDiffuse * diffuseStrength * vec3(texture(uMaterialDiffuse, TexCoords));
    vec3 specularAmt = spotSpecular * specularStrength * vec3(texture(uMaterialSpecular, TexCoords));

    ambientAmt *= attenuation * intensity;
    diffuseAmt *= attenuation * intensity;
    specularAmt *= attenuation * intensity;

    return ambientAmt + diffuseAmt + specularAmt;
}

void main()
{    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);

    vec3 result = CalcDirLight(
        uDirLightDirection,
        uDirLightAmbient,
        uDirLightDiffuse,
        uDirLightSpecular,
        norm,
        viewDir
    );

    for(int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        vec3 pointLightContribution = CalcPointLight(
            uPointLightPos[i],
            uPointLightConstant[i],
            uPointLightLinear[i],
            uPointLightQuadratic[i],
            uPointLightAmbient[i],
            uPointLightDiffuse[i],
            uPointLightSpecular[i],
            norm,
            FragPos,
            viewDir
        );
        //result += pointLightContribution; TODO
    }

    result += CalcSpotLight(
        uSpotLightPosition,
        uSpotLightDirection,
        uSpotLightCutoff,
        uSpotLightOuterCutoff,
        uSpotLightConstant,
        uSpotLightLinear,
        uSpotLightQuadratic,
        uSpotLightAmbient,
        uSpotLightDiffuse,
        uSpotLightSpecular,
        norm,
        FragPos,
        viewDir
    );

    FragColor = vec4(result, 1.0);
}
