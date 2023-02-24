#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3     inColor;
layout(location = 1) in vec2     inTexUV;
layout(location = 2) flat in int inTexIdx;

layout(set = 1, binding = 0) uniform sampler   samp;
layout(set = 1, binding = 1) uniform texture2D textures[80];

layout(location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = texture(sampler2D(textures[inTexIdx] , samp), inTexUV) * vec4(inColor, 1.0);
}