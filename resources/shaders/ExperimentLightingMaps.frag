#version 330 core

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform float uLightAmbient;
uniform float uLightDiffuse;
uniform float uLightSpecular;
uniform sampler2D uMaterialDiffuse;
uniform sampler2D uMaterialSpecular;
uniform float uMaterialShininess;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    vec3 diffuseColor = texture(uMaterialDiffuse, TexCoords).rgb;
    vec3 specularColor = texture(uMaterialSpecular, TexCoords).rgb;

    vec3 ambient = uLightAmbient * diffuseColor;

    // compute ambient strength (ambient + diffuse)
    vec3 frag2light = normalize(uLightPos - FragPos);
    float diffuseStrength = max(dot(Normal, frag2light), 0.0);
    vec3 diffuse = diffuseStrength * uLightDiffuse * diffuseColor;

    // specular
    vec3 frag2View = normalize(uViewPos - FragPos);
    vec3 light2frag = -frag2light;
    vec3 frag2LightReflection = reflect(light2frag, Normal);
    float specularStrength = pow(max(dot(frag2View, frag2LightReflection), 0.0), uMaterialShininess);
    vec3 specular = specularStrength * uLightSpecular * specularColor;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}

