#version 330 core

uniform sampler2D uInputImage;
uniform bool uHorizontal;
uniform float uWeights[5] = float[] (
    0.2270270270,
    0.1945945946,
    0.1216216216,
    0.0540540541,
    0.0162162162
);

in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    vec2 textelDims = 1.0/textureSize(uInputImage, 0);
    vec2 offsetPerStep = uHorizontal ? vec2(textelDims.x, 0.0) : vec2(0.0, textelDims.y);

    vec3 result = uWeights[0] * texture(uInputImage, TexCoord).rgb;
    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = float(i) * offsetPerStep;
        result += uWeights[i] * texture(uInputImage, TexCoord - offset).rgb;
        result += uWeights[i] * texture(uInputImage, TexCoord + offset).rgb;
    }
    FragColor = vec4(result, 1.0);
}
