This is a project I use to learn and experiment Vulkan.

## TLDR; 
The code might be rough at some places.
I don't do OOP while learning/exploring. More generally, I do not "Orient" code around objects in the Object Oriented sense.
You wont find a ``Game`` class with a ``game.run`` method, because there is only ever one instance of the game, we just call into it, so it does not need to be a class. In contrast. I might have a Sprite class, because that is resusable.

From my limited experience, I find that making everything a class only makes it harder to to keep track of the code and thus hurts its maintenance.
For that reason I write very procedural code with minimal indirection, only factoring things out that I intend to reuse, i.e buffer creation.
In general, I think it is way easier and more natural to first solve the problem at hand, get a better idea of the overall code and then let abstractions arise naturally and not try and fonce it upfront without any problem domain knowledge.

Just something that works well for me.

--- 
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
- Most Importantly: [Vulkan® 1.1.141 - A Specification (with all registered Vulkan extensions)](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html)
