#version 450

uniform mat4 MODEL;

in vec3 color;
out vec4 finalColor;

void main() {
    finalColor = vec4(pow(color, vec3(1/2.2)), 1);
}
