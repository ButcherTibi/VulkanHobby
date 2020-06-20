#version 450
#extension GL_ARB_separate_shader_objects : enable

// Inputs
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(binding = 0) uniform GPU_Uniform {
	vec4 data_0;
} uniform_buff;

// Outputs
layout(location = 0) out vec4 color_out;

void main()
{   
    float width = uniform_buff.data_0.x;
    float height = uniform_buff.data_0.y;

    vec4 vk_pos = vec4(
        pos.x / width * 2 - 1,
        pos.y / height * 2 - 1,
        pos.z,
        1
    );

    gl_Position = vk_pos;

    color_out = color;
}