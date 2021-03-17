@echo off

set PATH_VulkanSDK=C:/VulkanSDK/1.2.135.0/Bin

pushd shaders

%PATH_VulkanSDK%/glslc.exe vertexShader.vert -o vertexShader.vert.spv
%PATH_VulkanSDK%/glslc.exe fragmentShader.frag -o fragmentShader.frag.spv

%PATH_VulkanSDK%/glslc.exe coloredTriangle.vert -o coloredTriangle.vert.spv
%PATH_VulkanSDK%/glslc.exe coloredTriangle.frag -o coloredTriangle.frag.spv

%PATH_VulkanSDK%/glslc.exe triangleMesh.vert -o triangleMesh.vert.spv

popd

pause