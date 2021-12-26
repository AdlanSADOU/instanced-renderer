@echo off

set src_dir=src

set opts=-FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf -std:c++latest /MTd /MP
set code=%cd%/%src_dir%/*.cpp %cd%/%src_dir%/vkbootstrap/*.cpp %cd%/vendor/tinyobjloader/tiny_obj_loader.cc
set inc=-IC:/VulkanSDK/1.2.198.1/Include -I%cd%\%src_dir%\vkbootstrap -I%cd%\%src_dir%\ -I%cd%\%src_dir%\vma -I%cd%/vendor/SDL2/include -I%cd%/vendor/glm -I%cd%/vendor/tinyobjloader

set link_opts=-subsystem:console
set lib_paths=/LIBPATH:%cd%/vendor/SDL2/lib/x64 /LIBPATH:C:/VulkanSDK/1.2.198.1/Lib

if not exist build (mkdir build)

pushd build
cl %opts% %code% %inc% -Fevkgame.exe /link %link_opts% %lib_paths% SDL2.lib SDL2main.lib Vulkan-1.lib Shell32.lib Ole32.lib /time
popd
