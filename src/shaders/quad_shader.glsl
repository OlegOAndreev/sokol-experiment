#pragma sokol @vs vs_quad
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 uv;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = texcoord;
}
#pragma sokol @end

#pragma sokol @fs fs_quad
layout (binding = 0) uniform texture2D tex;
layout (binding = 0) uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
#pragma sokol @end

#pragma sokol @program quad_shader vs_quad fs_quad
