#version 150

in vec4 color;
in float y_value;
out vec4 fColor;

void main() {
    if (y_value < 0.1) discard;
    fColor = color;
}