This is a project I use to learn and experiment Vulkan.

The code might be very rough at some places, but overall I'd say, I don't bother with OOP, and while learning/exploring, I write very procedural code with minimal indirection, only factoring out things that repeat often, like buffer creation.

<img src="https://github.com/AdlanSADOU/Vulkan_Renderer/blob/master/.misc/v0.1.gif" width="44%" height="44%">


## Third party headers & libs
- ```SDL2```: cross platform Windowing & swapchain abstraction
- ```glm```: GLSL aligment compatible math types & functions
- ```imgui```: for future debug GUI
- ```stb_image```: single header lib for image (texture) loading
- ```tinyobjloader```: .obj 3D assets loading
- ```vma``` : Vulkan Memory Allocator by GPUOpen
- ```vkb``` : Instance, device, swapchain create (this dependency will soon be dropped)
## Sources:
- Excellent resource and a great read with some words of wisdom: [**I'm graphics and so can you** blog series by **Dustin Land** from idTech](https://www.fasterthan.life/blog/2017/7/11/i-am-graphics-and-so-can-you-part-1)
- Get the big picture: [Vulkan in 30 minutes](https://renderdoc.org/vulkan-in-30-minutes.html)
- The Vulkan Red book: [Vulkan-Programming-Guide](https://www.amazon.com/Vulkan-Programming-Guide-Official-Learning/dp/0134464540)
- The thing you should read the most: [Vulkan® 1.1.141 - A Specification (with all registered Vulkan extensions)](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html)
