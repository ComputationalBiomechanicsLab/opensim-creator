#version 330 core

// https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
// https://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors

uniform sampler2D uDiffuseMap;
uniform sampler2D uNormalMap;
uniform bool uEnableNormalMapping = true;
uniform float uAmbientStrength = 0.0;
uniform float uDiffuseStrength = 0.5;
uniform float uSpecularStrength = 1.0;
uniform float uSpecularShininess = 32.0;

in vec3 FragWorldPos;
in vec2 TexCoord;
in vec3 NormalTangentDir;  // from the vertex data, for non-normal-mapped comparison
in vec3 LightTangentPos;
in vec3 ViewTangentPos;
in vec3 FragTangentPos;

out vec4 FragColor;

float CalcBrightnessWithoutNormalMapping()
{
    // perform shading calculations (phong)
    vec3 lightTangentDir = normalize(LightTangentPos - FragTangentPos);
    vec3 viewTangentDir = normalize(ViewTangentPos - FragTangentPos);
    vec3 reflectTangentDir = reflect(-lightTangentDir, NormalTangentDir);
    vec3 halfwayTangentDir = normalize(lightTangentDir + viewTangentDir);

    float diffuseStrength = uDiffuseStrength * max(dot(lightTangentDir, NormalTangentDir), 0.0);
    float specularStrength = uSpecularStrength * pow(max(dot(NormalTangentDir, halfwayTangentDir), 0.0), uSpecularShininess);

    return uAmbientStrength + diffuseStrength + specularStrength;
}

float CalcBrightnessWithNormalMapping()
{
    // read normal in range [0, 1] from texture
    vec3 normalTangentDir = texture(uNormalMap, TexCoord).rgb;
    normalTangentDir = normalize(2.0*normalTangentDir - 1.0);  // remap to [-1,1]

    // perform shading calculations (phong)
    vec3 lightTangentDir = normalize(LightTangentPos - FragTangentPos);
    vec3 viewTangentDir = normalize(ViewTangentPos - FragTangentPos);
    vec3 reflectTangentDir = reflect(-lightTangentDir, normalTangentDir);
    vec3 halfwayTangentDir = normalize(lightTangentDir + viewTangentDir);

    float diffuseStrength = uDiffuseStrength * max(dot(lightTangentDir, normalTangentDir), 0.0);
    float specularStrength = uSpecularStrength * pow(max(dot(normalTangentDir, halfwayTangentDir), 0.0), uSpecularShininess);

    return uAmbientStrength + diffuseStrength + specularStrength;
}

void main()
{
    // read diffuse color from texture
    vec3 diffuseColor = texture(uDiffuseMap, TexCoord).rgb;

    float brightness = uEnableNormalMapping ?
        CalcBrightnessWithNormalMapping() :
        CalcBrightnessWithoutNormalMapping();

    FragColor = vec4(brightness * diffuseColor, 1.0);
}