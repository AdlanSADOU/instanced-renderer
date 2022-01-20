
<!-- <img src="https://github.com/AdlanSADOU/Vulkan_Renderer/blob/master/.misc/v0.1.gif" width="44%" height="44%"> -->

# Quick Warning Notice
This project is still under heavy development and in early stage. I'm still working on & exploring techniques, thus the API is not yet fleshed out.
This is not much of a library for now and I don't expect anybody to use it at this time. Do so at your own risk and peril. 

Please note that the examples might not run on your particular hardware **for now** because I make a few assumptions. This will be fixed of course going forward.

## Features

- Instanced rendering 
- Array of textures: indexed inside fragment shader per primitive instance
- bare bones compute dispatch

**Result**: as of now, a 2D instanced renderer able to display more than 2 million sprites at high framerates (60+ fps) on entry level hardware, like a GTX 1050 ti.
And twice as much on a GTX 1070.
## Dependencies

- ```SDL2```: cross platform Windowing & swapchain abstraction
- ```glm```: GLSL aligment compatible math types & functions
- ```imgui```: for future debug GUI
- ```stb_image```: single header lib for image (texture) loading
- ```vma``` : Vulkan Memory Allocator by GPUOpen
