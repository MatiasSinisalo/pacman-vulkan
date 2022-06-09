#version 450


layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 1) uniform ubo {
    mat4 scale;
} uniformData;

layout( push_constant ) uniform constants
{
	mat4 model;
} PushConstants;



layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = uniformData.scale * PushConstants.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
