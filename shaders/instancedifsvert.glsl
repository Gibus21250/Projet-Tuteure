#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 0) out vec3 fragColor;

layout (binding = 0) uniform UniformBufferObject
{
    mat4 t[65536];  //maxUniformBufferRange: 65536
} transforms;

layout (push_constant) uniform constants
{
    uint nbIteration;
    uint maxInstance;
    uint nbTransformation;
    uint padding;
    mat4 render_matrix;
} PushConstants;

void main()
{
    float currentValue = float(gl_InstanceIndex) / float(PushConstants.maxInstance - 1);
    float start = currentValue;
    float f_nbTransform = float(PushConstants.nbTransformation);
    float segmentSize = float(1.0f) / f_nbTransform;

    mat4 model = mat4(1);

    for (int i = 0; i < PushConstants.nbIteration; i++)
    {
        //Get the indice of the transfom to apply
        float f_no = floor(currentValue * f_nbTransform);
        uint no = min(uint(f_no), PushConstants.nbTransformation-1);
        model = model * transpose(transforms.t[no]);

        //Remap the value between 0 and 1 for the next iteration
        float lower = float(no) * segmentSize;
        currentValue = (currentValue - lower) / segmentSize;
    }

    gl_Position = PushConstants.render_matrix * model * vec4(inPosition, 1.0);
    fragColor = vec3(1, start, 1.0);
}