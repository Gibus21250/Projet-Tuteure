#version 450

uniform mat4 MVP;

layout(std140, binding = 0) uniform TransformsBlock {
    mat4 m[3];
};

layout(std140, binding = 1) uniform DrawInfoBlock {
    uint nbIteration;
    uint maxInstance;
    uint nbTransformation;
    uint padding;
};

layout (location = 0) in vec3 inPosition;

out vec3 color;

void main()
{
    float currentValue = float(gl_InstanceID) / float(maxInstance - 1);
    color = vec3(currentValue, 0, 0);

    float f_nbTransform = float(nbTransformation);
    float segmentSize = float(1.0f) / f_nbTransform;

    mat4 model = mat4(1);

    for (int i = 0; i < nbIteration; i++)
    {
        //Get the indice of the transfom to apply
        float f_no = floor(currentValue * f_nbTransform);
        uint no = min(uint(f_no), nbTransformation - 1);

        model = model * transpose(m[no]);

        //Remap the value between 0 and 1 for the next iteration
        float lower = float(no) * segmentSize;
        currentValue = (currentValue - lower) / segmentSize;
    }

    gl_Position = MVP * model * vec4(inPosition, 1.0);

}
