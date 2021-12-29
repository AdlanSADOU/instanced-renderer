scratch

https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_InstanceID.xhtml

gl_InstanceID is a vertex language input variable that holds the integer index of the current primitive in an instanced draw command such as glDrawArraysInstanced.
If the current primitive does not originate from an instanced draw command, the value of gl_InstanceID is zero.


void vkCmdDrawIndexed(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance);

- commandBuffer is the command buffer into which the command is recorded.
- indexCount is the number of vertices to draw.
- instanceCount is the number of instances to draw.
- firstIndex is the base index within the index buffer.
- vertexOffset is the value added to the vertex index before indexing into the vertex buffer.
- firstInstance is the instance ID of the first instance to draw.


void vkCmdBindIndexBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType);

- commandBuffer is the command buffer into which the command is recorded.
- buffer is the buffer being bound.
- offset is the starting offset in bytes within buffer used in index buffer address calculations.
- indexType is a VkIndexType value specifying the size of the indices.


        struct InstanceData {
         glm::vec3 pos;
         glm::vec3 rot;
         float scale;
         uint32_t texIndex;
        };
        // Contains the instanced data
        struct InstanceBuffer {
         VkBuffer buffer = VK_NULL_HANDLE;
         VkDeviceMemory memory = VK_NULL_HANDLE;
         size_t size = 0;
         VkDescriptorBufferInfo descriptor;
        } instanceBuffer;


indexBuffer
    uint indices[3]
vertexBuffer            (vkBuffer | vkCmdBindVertexBuffers())
    float3 inPos;
    float3 inNormal;
    float2 inUV;
    float3 inColor;
    xint pad[13];

InstanceBuffer          (vkBuffer | vkCmdBindVertexBuffers())
    float3 instancePos;
    float3 instanceRot;
    float1 instanceScale;
    int1 instanceTexIndex;

VkVertexInputState for instance data

---

## Descriptors

### struct types

    VkDescriptorPoolCreateInfo
    VkDescriptorSetLayoutBinding
    VkDescriptorSetLayoutCreateInfo
    VkDescriptorSetAllocateInfo
    VkDescriptorBufferInfo
    VkWriteDescriptorSet

### functions

vkCreateDescriptorPool
vkCreateDescriptorSetLayout
vkAllocateDescriptorSets
vkUpdateDescriptorSets

### descriptor types

    VK_DESCRIPTOR_TYPE_SAMPLER = 0,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT = 1000138000,
    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000,
    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV = 1000165000,
    VK_DESCRIPTOR_TYPE_MUTABLE_VALVE = 1000351000,
    VK_DESCRIPTOR_TYPE_MAX_ENUM = 0x7FFFFFFF


any way to translate instanced meshes invidually with push constants?
forget that
instanced drawing is not what you're looking for
we need batch rendering
infact we'll have geometry in one single buffer + transform
buffer has to be dynamic now

 * 32^3

sizeof(Vertex) = 108 = (3*(3*4) * 3)
triange_count = 8^3

