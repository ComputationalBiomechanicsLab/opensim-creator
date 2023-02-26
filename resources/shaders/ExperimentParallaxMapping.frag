#version 330 core

uniform sampler2D uDiffuseMap;
uniform sampler2D uNormalMap;
uniform sampler2D uDisplacementMap;
uniform bool uEnableMapping = true;
uniform float uHeightScale;

in vec3 FragWorldPos;
in vec2 TexCoord;
in vec3 NormalTangentDir;  // from the vertex data, for non-normal-mapped comparison
in vec3 LightTangentPos;
in vec3 ViewTangentPos;
in vec3 FragTangentPos;

out vec4 FragColor;

vec2 ParallaxMapping(vec2 texCoord, vec3 tangentViewDir)
{ 
    float height = texture(uDisplacementMap, texCoord).r;
    return texCoord -  (uHeightScale * height * tangentViewDir.xy);
}

vec3 CalcFragColorWithMapping()
{
    // offset texture coordinates with Parallax Mapping
    vec3 tangentFrag2ViewDir = normalize(ViewTangentPos - FragTangentPos);

    vec2 actualTexCoord = ParallaxMapping(TexCoord, tangentFrag2ViewDir);
    if (actualTexCoord.x > 1.0 ||
        actualTexCoord.y > 1.0 ||
        actualTexCoord.x < 0.0 ||
        actualTexCoord.y < 0.0)
    {
        discard;
    }

    // read normal in range [0, 1] from normal map
    vec3 tangentNormalDir = texture(uNormalMap, actualTexCoord).rgb;
    tangentNormalDir = normalize(2.0*tangentNormalDir - 1.0);  // remap to [-1,1]

    // perform shading calculations (phong)
    vec3 tangentFrag2LightDir = normalize(LightTangentPos - FragTangentPos);
    vec3 tangentLight2FragReflectionDir = reflect(-tangentFrag2LightDir, tangentNormalDir);
    vec3 tangentHalfwayDir = 0.5 * (tangentFrag2LightDir + tangentFrag2ViewDir);

    float ambientStrength = 0.1;
    float diffuseStrength = max(dot(tangentFrag2LightDir, tangentNormalDir), 0.0);
    float specularStrength = 0.2 * pow(max(dot(tangentNormalDir, tangentHalfwayDir), 0.0), 32.0);
    float strength = ambientStrength + diffuseStrength + specularStrength;

    return strength * texture(uDiffuseMap, actualTexCoord).rgb;
}

vec3 CalcFragColorWithoutMapping()
{
    // perform shading calculations (phong)
    vec3 lightTangentDir = normalize(LightTangentPos - FragTangentPos);
    vec3 viewTangentDir = normalize(ViewTangentPos - FragTangentPos);
    vec3 reflectTangentDir = reflect(-lightTangentDir, NormalTangentDir);
    vec3 halfwayTangentDir = normalize(lightTangentDir + viewTangentDir);

    float ambientStrength = 0.1;
    float diffuseStrength = max(dot(lightTangentDir, NormalTangentDir), 0.0);
    float specularStrength = 0.2 * pow(max(dot(NormalTangentDir, halfwayTangentDir), 0.0), 32.0);

    float strength = ambientStrength + diffuseStrength + specularStrength;

    return strength*texture(uDiffuseMap, TexCoord).rgb;
}

void main()
{
    vec3 color = uEnableMapping ? CalcFragColorWithMapping() : CalcFragColorWithoutMapping();
    FragColor = vec4(color, 1.0);
}