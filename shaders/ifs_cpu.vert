#version 450

uniform mat4 MVP;

layout(std430, binding = 0) buffer TransformBuffer {
    mat4 instanceMatrices[];
};

layout (location = 0) in vec3 inPosition;

out vec3 color;

void main()
{
    mat4 model = instanceMatrices[gl_InstanceID];
    gl_Position = MVP * model * vec4(inPosition, 1.0);

}
