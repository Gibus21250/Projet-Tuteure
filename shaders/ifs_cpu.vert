#version 450

uniform mat4 MVP;
uniform uint numInstances;

layout(std430, binding = 0) restrict readonly buffer TransformBuffer {
    mat4 transform[];
} instancedSSBO;

layout (location = 0) in vec3 inPosition;

out vec3 color;

void main()
{
    color = vec3(float(gl_InstanceID) / numInstances, 0, 0);
    mat4 model = instancedSSBO.transform[gl_InstanceID];

    gl_Position = MVP * model * vec4(inPosition, 1.0);
}
