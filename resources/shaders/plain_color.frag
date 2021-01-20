#version 330 core

out vec4 FragColor;

uniform vec3 uRgb;

void main(void) {
    FragColor = vec4(uRgb, 1.0);
}
