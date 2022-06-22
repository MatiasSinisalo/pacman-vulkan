#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler texSampler;
layout(binding = 2) uniform texture2D objectTexture[5];

layout( push_constant ) uniform constants
{
	mat4 model;
    int textureIndex;
} PushConstants;




void main() {
   outColor = texture(sampler2D(objectTexture[PushConstants.textureIndex], texSampler), fragTexCoord);
}