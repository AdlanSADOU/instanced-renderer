#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 3) in vec3 iPos;
layout (location = 4) in vec3 iRot;

layout (location = 0) out vec3 outColor;

layout(set = 0, binding = 0) uniform  CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

layout (push_constant) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

void main()
{

    vec4 vertex_pos = vec4(vPosition, 1.0);
    vec4 pos = vec4(vertex_pos.xyz + iPos, 1.0);

	mat4 tranformMatrix = (cameraData.viewproj  ) ;
	gl_Position = tranformMatrix * pos;
	outColor = vColor;
}
