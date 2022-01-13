# Vulkan 1.2

## Some great resources

- [Awesome Vulkan](https://github.com/vinjn/awesome-vulkan)
- [Vulkan Resouces - Specs](https://www.vulkan.org/learn#key-resources)
- [Mike Bailey's Vulkan Page](http://web.engr.oregonstate.edu/~mjb/vulkan/)
- [Vulkanised 2021](https://www.khronos.org/events/vulkanised-2021)
- [Writing an efficient vulkan renderer - A.Kapoulkine](https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/)
- [Vulkan Dos and Donts - Nvidia](https://developer.nvidia.com/blog/vulkan-dos-donts/)
## quick notes

use case:

- common
  - (if multiple use cases are possible, please specify them)

- intended

- only

    VK_KHR_dynamic_rendering
we might wanna check out; "allows developers to tell the API to start rendering with no render pass or framebuffer objects"

## Memory

- Vulkan expects allocations to be aligned correctly

## [VkInstance](https://www.vulkan.org/learn#key-resources)

    There is no global state in Vulkan and all per-application state is stored in a VkInstance object. Creating a VkInstance object initializes the Vulkan library and allows the application to pass information about itself to the implementation.

    - verifies that the requested layers exist
    - verifies that the requested extensions are supported
    - if any requested extension is not supported, vkCreateInstance must return     VK_ERROR_EXTENSION_NOT_PRESENT
    - If a requested extension is only supported by a layer, both the layer and the extension need to be specified at vkCreateInstance time for the creation to succeed.

Basically, the instance represents your application, makes sure that the validation layers & instance extentions you requested are valid.

## [VkPhysicalDevice](https://www.khronos.org/registry/vulkan/specs/1.2/html/chap5.html)

    Once Vulkan is initialized, devices and queues are the primary objects used to interact with a Vulkan implementation.

    Vulkan separates the concept of physical and logical devices. A physical device usually represents a single complete implementation of Vulkan (excluding instance-level functionality) available to the host, of which there are a finite number (in other words, your beloved RTX 3090). A logical device represents an instance of that implementation with its own state and resources independent of other logical devices.

## PushConstants

## StorageBuffer

## UniformBuffer

## TexelBuffer

## Samplers

### [Sampler](https://www.khronos.org/registry/vulkan/specs/1.2/html/chap14.html#descriptorsets-sampler)

- VK_DESCRIPTOR_TYPE_SAMPLER

### [Sampled Image](https://www.khronos.org/registry/vulkan/specs/1.2/html/chap14.html#descriptorsets-sampledimage)

### [Combined Image Sampler](https://www.khronos.org/registry/vulkan/specs/1.2/html/chap14.html#descriptorsets-combinedimagesampler)

- VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER


## Sparse Memory Resources

## DescriptorSets

---

## Shaders

### [Static Use](https://www.khronos.org/registry/vulkan/specs/1.2/html/chap9.html#shaders-staticuse)

### Compute

get the queue index > vkGetPhysicalDeviceQueueFamilyProperties
specify the queue index > vkDeviceQueueCreateInfo
get the device queue > vkGetDeviceQueue
