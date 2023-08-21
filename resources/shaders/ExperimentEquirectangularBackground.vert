#version 330 core

uniform mat4 uViewMat;
uniform mat4 uProjMat;

layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

void main()
{
    WorldPos = aPos;

	mat4 rotView = mat4(mat3(uViewMat));
	vec4 clipPos = uProjMat * rotView * vec4(WorldPos, 1.0);

	gl_Position = clipPos.xyww;
}