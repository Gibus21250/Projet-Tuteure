#version 450

uniform mat4 MVP;
uniform uint nbIteration;

struct StateData {
    uint nbTransform;       //Number of transform
    uint padding;           //Number of tranform to jump above for the tranform array
};

//Buffer containing states informations, and a special padding value to jump to state's tranforms info
layout(std430, binding = 0) restrict readonly buffer StateInfo {
    StateData states[];
} stateInfo;

struct TransformData {
    mat4 mat;
    uint nextState;
};

//Buffer containing all matrices and the next state
layout(std430, binding = 1) restrict readonly buffer TransformInfo {
    TransformData transforms[];
} transformInfo;

//Buffer of all encode path
layout(std430, binding = 2) restrict readonly buffer CodeInfo {
    float code[];
} codeInfo;

layout (location = 0) in vec3 inPosition;
out vec3 color;

void main()
{
    float code = codeInfo.code[gl_InstanceID];
    color = vec3(code, 0, 0);

    mat4 model = mat4(1); // Eye
    uint currentState = 0; //Start state 0

    for (uint j = 0; j < nbIteration; j++) {

        const float stateStep = 1.0f / float(stateInfo.states[currentState].nbTransform);
        const uint transformIndex = uint(floor(code / stateStep));

        //Accumulate transforms
        model = transformInfo.transforms[stateInfo.states[currentState].padding + transformIndex].mat * model;
        //Get next state
        currentState = transformInfo.transforms[stateInfo.states[currentState].padding + transformIndex].nextState;

        //Rescale encoded value for the next decoding iteration
        const float lower = float(transformIndex) * stateStep;
        const float upper = float(transformIndex + 1) * stateStep;
        code = (code - lower) / (upper - lower);

    }

    gl_Position = MVP * model * vec4(inPosition, 1.0);

}
