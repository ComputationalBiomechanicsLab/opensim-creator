#version 330 core

uniform samplerCube uSkybox;

in vec3 TexCoords;

out vec4 FragColor;

void main()
{
    // Map model space (right-handed cube) to "cube map texture selection space"
    //     See:  https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf ("Cube Map Texture Selection")
    FragColor = texture(uSkybox, vec3(TexCoords.x, -TexCoords.yz));
}
