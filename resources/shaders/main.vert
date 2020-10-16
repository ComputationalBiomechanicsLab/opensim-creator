#version 330 core

// A non-texture-mapping Gouraud shader that just adds some basic
// (diffuse / specular) highlights to the model

uniform mat4 projMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 normalMat;

uniform vec4 rgba;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

layout (location = 0) in vec3 location;
layout (location = 1) in vec3 in_normal;

out vec4 frag_color;

void main() {
    // apply xforms (model, view, perspective) to vertex
    gl_Position = projMat * viewMat * modelMat * vec4(location, 1.0f);

    // passthrough the normals (used by frag shader)
    vec3 normal = normalize(mat3(normalMat) * in_normal);
    // pass fragment pos in world coordinates to frag shader
    vec3 frag_pos = vec3(modelMat * vec4(location, 1.0f));


    // normalized surface normal
    vec3 norm = normalize(normal);
    // direction of light, relative to fragment, in world coords
    vec3 light_dir = normalize(lightPos - frag_pos);

    // strength of diffuse (Phong model) lighting
    float diffuse_strength = 0.3f;
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diffuse_strength * diff * lightColor;

    // strength of ambient (Phong model) lighting
    float ambient_strength = 0.5f;
    vec3 ambient = ambient_strength * lightColor;

    // strength of specular (Blinn-Phong model) lighting
    // Blinn-Phong is a modified Phong model that
    float specularStrength = 0.1f;
    vec3 lightDir = normalize(lightPos - frag_pos);
    vec3 viewDir = normalize(viewPos - frag_pos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-light_dir, norm);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 rgb = (ambient + diffuse + specular) * rgba.rgb;
    frag_color = vec4(rgb, rgba.a);
}
