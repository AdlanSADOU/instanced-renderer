
This project is still under heavy development and in early stage. Even though almost all critical features are there, the API is not yet fleshed out.
So pretty big changes and refactorings are to come.

--- 

## Features

- Instanced quad rendering
- Array of textures: indexed inside fragment shader per quad instance

**Result**: a 2D renderer able to render more than 2 million sprites at high framerates (60+ fps) on relatively modern GPUs with only 1 draw call

**Next feature**: Moving transform updates from CPU to GPU via compute shaders

---

<img src="https://github.com/AdlanSADOU/Vulkan_Renderer/blob/master/.misc/v0.1.gif" width="44%" height="44%">


## Dependencies

- ```SDL2```: cross platform Windowing & swapchain abstraction
- ```glm```: GLSL aligment compatible math types & functions
- ```imgui```: for future debug GUI
- ```stb_image```: single header lib for image (texture) loading
- ```vma``` : Vulkan Memory Allocator by GPUOpen

## Sources:

- Excellent resource and a great read with some words of wisdom: [**I'm graphics and so can you** blog series by **Dustin Land** from idTech](https://www.fasterthan.life/blog/2017/7/11/i-am-graphics-and-so-can-you-part-1)
- Get the big picture: [Vulkan in 30 minutes](https://renderdoc.org/vulkan-in-30-minutes.html)
- The Vulkan Red book: [Vulkan-Programming-Guide](https://www.amazon.com/Vulkan-Programming-Guide-Official-Learning/dp/0134464540)
- Most Importantly: [Vulkan® 1.1.141 - A Specification (with all registered Vulkan extensions)](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html)
