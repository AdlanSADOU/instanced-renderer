#version 460
#extension GL_ARB_separate_shader_objects : enable

//shader input
layout(location = 0) in vec4     inColor;
layout(location = 1) in vec2     inTexUV;

// layout(set = 1, binding = 0) uniform sampler   samp;
// layout(set = 1, binding = 1) uniform texture2D textures[80];


//output write
layout(location = 0) out vec4 outFragColor;


void main()
{
	//return color
    // vec4(inColor,1.0f);
	// outFragColor = texture(sampler2D(textures[inTexIdx] , samp), inTexUV) * vec4(inColor, 1.0);
    outFragColor = inColor;

}