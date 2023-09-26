#version 330 core

uniform vec2 uRescale;
uniform vec2 uOffset;
uniform int uNumIterations;

in vec2 aFragPos;
out vec4 LFragment;

void main()
{
    vec2 xy0 = uRescale*aFragPos + uOffset;
    float x = 0.0;
    float y = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;

    int iter = 0;
    while (iter < uNumIterations && x2+y2 <= 4.0)
    {
        y = 2*x*y + xy0.y;
        x = x2-y2 + xy0.x;
        x2 = x*x;
        y2 = y*y;
        iter++;
    }

    float brightness = iter == uNumIterations ? 0.0 : float(iter)/float(uNumIterations);
    LFragment = vec4(brightness, brightness, brightness, 1.0);
}
