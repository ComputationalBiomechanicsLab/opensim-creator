#version 330 core

uniform vec4 uLightColor;

in vec3 FragWorldPos;
in vec3 NormalWorld;
in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main()
{
    FragColor = uLightColor;

    // thresholded write
    if (dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722)) > 1.0)
    {
        BrightColor = vec4(FragColor.rgb, 1.0);
    }
    else
    {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
