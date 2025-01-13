#version 450

uniform mat4 lightSpaceMatrix;
uniform mat4 MODEL;

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = lightSpaceMatrix * MODEL * vec4(aPos, 1.0);
}