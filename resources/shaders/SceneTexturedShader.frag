#version 330 core

uniform sampler2D uDiffuseTexture;
uniform float uNear;
uniform float uFar;

in vec2 TexCoord;
in vec4 GouraudBrightness;
out vec4 Color0Out;

float LinearizeDepth(float depth)
{
    // from: https://learnopengl.com/Advanced-OpenGL/Depth-testing
    //
    // only really works with perspective cameras: orthogonal cameras
    // don't need this unprojection math trick

    float z = depth * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

void main()
{
    Color0Out = GouraudBrightness * texture(uDiffuseTexture, TexCoord);
    Color0Out.a = Color0Out.a - (LinearizeDepth(gl_FragCoord.z) / uFar);
    Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
}