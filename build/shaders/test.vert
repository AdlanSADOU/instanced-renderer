#version 450

layout(set = 0, binding = 0, std140) uniform UBO
{
    mat4 projection;
    mat4 modelview;
    vec4 lightPos;
    float locSpeed;
    float globSpeed;
} ubo;

layout(location = 1) out vec3 outColor;
layout(location = 3) in vec3 inColor;
layout(location = 2) out vec3 outUV;
layout(location = 2) in vec2 inUV;
layout(location = 7) in int instanceTexIndex;
layout(location = 5) in vec3 instanceRot;
layout(location = 0) in vec3 inPos;
layout(location = 6) in float instanceScale;
layout(location = 4) in vec3 instancePos;
layout(location = 0) out vec3 outNormal;
layout(location = 1) in vec3 inNormal;
layout(location = 4) out vec3 outLightVec;
layout(location = 3) out vec3 outViewVec;

void main()
{
    outColor = inColor;
    outUV = vec3(inUV, float(instanceTexIndex));
    float s = sin(instanceRot.x + ubo.locSpeed);
    float c = cos(instanceRot.x + ubo.locSpeed);

    mat3 mx;
    mx[0] = vec3(c, s, 0.0);
    mx[1] = vec3(-s, c, 0.0);
    mx[2] = vec3(0.0, 0.0, 1.0);

    s = sin(instanceRot.y + ubo.locSpeed);
    c = cos(instanceRot.y + ubo.locSpeed);
    mat3 my;
    my[0] = vec3(c, 0.0, s);
    my[1] = vec3(0.0, 1.0, 0.0);
    my[2] = vec3(-s, 0.0, c);

    s = sin(instanceRot.z + ubo.locSpeed);
    c = cos(instanceRot.z + ubo.locSpeed);
    mat3 mz;
    mz[0] = vec3(1.0, 0.0, 0.0);
    mz[1] = vec3(0.0, c, s);
    mz[2] = vec3(0.0, -s, c);

    mat3 rotMat = (mz * my) * mx;
    s = sin(instanceRot.y + ubo.globSpeed);
    c = cos(instanceRot.y + ubo.globSpeed);
    mat4 gRotMat;
    gRotMat[0] = vec4(c, 0.0, s, 0.0);
    gRotMat[1] = vec4(0.0, 1.0, 0.0, 0.0);
    gRotMat[2] = vec4(-s, 0.0, c, 0.0);
    gRotMat[3] = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 locPos = vec4(inPos * rotMat, 1.0);
    vec4 pos = vec4((locPos.xyz * instanceScale) + instancePos, 1.0);
    gl_Position = ((ubo.projection * ubo.modelview) * gRotMat) * pos;

    mat4 _199 = ubo.modelview * gRotMat;
    outNormal = (mat3(_199[0].xyz, _199[1].xyz, _199[2].xyz) * inverse(rotMat)) * inNormal;
    pos = ubo.modelview * vec4(inPos + instancePos, 1.0);
    vec3 lPos = mat3(ubo.modelview[0].xyz, ubo.modelview[1].xyz, ubo.modelview[2].xyz) * ubo.lightPos.xyz;
    outLightVec = lPos - pos.xyz;
    outViewVec = -pos.xyz;
}
