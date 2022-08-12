#version 330 core

uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform float uNear;
uniform float uFar;

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
    Color0Out = GouraudBrightness * uDiffuseColor;
    Color0Out.a *= 1.0 - (LinearizeDepth(gl_FragCoord.z) / uFar);  // fade into background at high distances
    Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
}