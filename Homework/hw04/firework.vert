#version 150

in vec4 vColor;
in vec3 vVelocity;

out vec4 color;
out float y_value;

uniform int time;

uniform mat4 ModelView;
uniform mat4 Projection;

void main() {
    vec4 init_pos = vec4(0.0, 0.1, 0.0, 1.0);
    vec4 pos =
        vec4(0.001 * vVelocity.x * time,
             0.001 * vVelocity.y * time - 0.5 * 0.49 * pow((0.001 * time), 2),
             0.001 * vVelocity.z * time, 0) +
        init_pos;

    gl_Position = Projection * ModelView * pos;

    color = vColor;
    y_value = pos.y;
}