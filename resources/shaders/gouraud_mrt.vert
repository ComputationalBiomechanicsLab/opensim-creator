#version 330 core

// gouraud_shader:
//
// performs lighting calculations per vertex (Gouraud shading), rather
// than per frag ((Blinn-)Phong shading)

uniform mat4 uProjMat;
uniform mat4 uViewMat;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform bool uSkipVP = false;

layout (location = 0) in vec3 aLocation;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4x3 aModelMat;
layout (location = 7) in mat3 aNormalMat;
layout (location = 10) in vec4 aRgba0;
layout (location = 11) in vec3 aRgb1;

out vec4 GouraudBrightness;
out vec4 Rgba0;
out vec3 Rgb1;
out vec2 TexCoord;

const float ambientStrength = 0.5f;
const float diffuseStrength = 0.3f;
const float specularStrength = 0.1f;
const float shininess = 32;

void main() {
    mat4 modelMat = mat4(vec4(aModelMat[0], 0), vec4(aModelMat[1], 0), vec4(aModelMat[2], 0), vec4(aModelMat[3], 1));

    if (uSkipVP) {
        gl_Position = modelMat * vec4(aLocation, 1.0);
    } else {
        gl_Position = uProjMat * uViewMat * modelMat * vec4(aLocation, 1.0);
    }

    vec3 normalDir = normalize(aNormalMat * aNormal);
    vec3 fragPos = vec3(modelMat * vec4(aLocation, 1.0));
    vec3 frag2lightDir = normalize(uLightPos - fragPos);
    vec3 frag2viewDir = normalize(uViewPos - fragPos);

    vec3 ambientComponent = ambientStrength * uLightColor;

    float diffuseAmount = max(dot(normalDir, frag2lightDir), 0.0);
    vec3 diffuseComponent = diffuseStrength * diffuseAmount * uLightColor;

    vec3 halfwayDir = normalize(frag2lightDir + frag2viewDir);
    float specularAmmount = pow(max(dot(normalDir, halfwayDir), 0.0), shininess);
    vec3 specularComponent = specularStrength * specularAmmount * uLightColor;

    vec3 lightStrength = ambientComponent + diffuseComponent + specularComponent;

    GouraudBrightness = vec4(uLightColor * lightStrength, 1.0);
    Rgba0 = aRgba0;
    Rgb1 = aRgb1;
    TexCoord = aTexCoord;
}
