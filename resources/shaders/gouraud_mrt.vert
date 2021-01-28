#version 330 core

// gouraud_shader:
//
// performs lighting calculations per vertex (Gouraud shading), rather
// than per frag ((Blinn-)Phong shading)

layout (location = 0) in vec3 aLocation;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in mat4 aModelMat;
layout (location = 6) in mat4 aNormalMat;
layout (location = 10) in vec4 aRgba0;
layout (location = 11) in vec4 aRgba1;

uniform mat4 uProjMat;
uniform mat4 uViewMat;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

out vec4 Color0in;
out vec4 Color1in;

const float ambientStrength = 0.5f;
const float diffuseStrength = 0.3f;
const float specularStrength = 0.1f;
const float shininess = 32;

void main() {
    gl_Position = uProjMat * uViewMat * aModelMat * vec4(aLocation, 1.0);

    vec3 normalDir = normalize(mat3(aNormalMat) * aNormal);
    vec3 fragPos = vec3(aModelMat * vec4(aLocation, 1.0));
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

    Color0in = vec4(lightRgb, 1.0) * aRgba0;
    Color1in = aRgba1; // passthrough
}
