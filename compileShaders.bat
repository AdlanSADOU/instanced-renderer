@echo off

set PATH_VulkanSDK_bin=%cd%/vendor/Vulkan/1.2.198.1/Bin

pushd shaders

%PATH_VulkanSDK_bin%/glslc.exe textureArray.frag -o textureArray.frag.spv
%PATH_VulkanSDK_bin%/glslc.exe mesh.frag -o mesh.frag.spv

%PATH_VulkanSDK_bin%/glslc.exe quad.vert -o quad.vert.spv
%PATH_VulkanSDK_bin%/glslc.exe mesh.vert -o mesh.vert.spv

%PATH_VulkanSDK_bin%/glslc.exe basicComputeShader.comp -o basicComputeShader.comp.spv
%PATH_VulkanSDK_bin%/glslc.exe writeRenderTarget.comp -o writeRenderTarget.comp.spv

popd

pause