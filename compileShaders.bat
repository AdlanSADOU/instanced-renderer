@echo off

set PATH_VulkanSDK_bin=%cd%/vendor/Vulkan/1.2.198.1/Bin

pushd shaders

%PATH_VulkanSDK_bin%/glslc.exe vertexShader.vert -o vertexShader.vert.spv
%PATH_VulkanSDK_bin%/glslc.exe fragmentShader.frag -o fragmentShader.frag.spv

%PATH_VulkanSDK_bin%/glslc.exe coloredTriangle.vert -o coloredTriangle.vert.spv
%PATH_VulkanSDK_bin%/glslc.exe coloredTriangle.frag -o coloredTriangle.frag.spv
%PATH_VulkanSDK_bin%/glslc.exe textureArray.frag -o textureArray.frag.spv

%PATH_VulkanSDK_bin%/glslc.exe triangleMesh.vert -o triangleMesh.vert.spv

popd

pause