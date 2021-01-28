#version 330 core

// gouraud_shader:
//
// performs lighting calculations per vertex (Gouraud shading), rather
// than per frag ((Blinn-)Phong shading)

layout (location = 0) in vec3 aLocation;
layout (location = 1) in vec3 aNormal;

uniform mat4 uProjMat;
uniform mat4 uViewMat;
uniform mat4 uModelMat;
uniform mat4 uNormalMat;
uniform vec4 uRgba0;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

out vec4 FragColor;

const float ambientStrength = 0.5f;
const float diffuseStrength = 0.3f;
const float specularStrength = 0.1f;
const float shininess = 32;

void main() {
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aLocation, 1.0);

    vec3 normalDir = normalize(mat3(uNormalMat) * aNormal);
    vec3 fragPos = vec3(uModelMat * vec4(aLocation, 1.0));
    vec3 frag2lightDir = normalize(uLightPos - fragPos);
    vec3 frag2viewDir = normalize(uViewPos - fragPos);

    vec3 ambientComponent = ambientStrength * uLightColor;

    float diffuseAmount = max(dot(normalDir, frag2lightDir), 0.0);
    vec3 diffuseComponent = diffuseStrength * diffuseAmount * uLightColor;

    vec3 halfwayDir = normalize(frag2lightDir + frag2viewDir);
    float specularAmmount = pow(max(dot(normalDir, halfwayDir), 0.0), shininess);
    vec3 specularComponent = specularStrength * specularAmmount * uLightColor;

    vec3 lightStrength = ambientComponent + diffuseComponent + specularComponent;
    vec3 lightRgb = uLightColor * lightStrength;

    FragColor = vec4(lightRgb, 1.0) * uRgba0;
}
