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
-----

vkCmdDrawIndirect: buffer + offset + (stride * index)
stride[22 ] * (drawCount[10] - 1) + offset[24] + sizeof(VkDrawIndirectCommand)[16] = 196

VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT

  uint32_t    vertexCount;
    uint32_t    instanceCount;
    uint32_t    firstVertex;
    uint32_t    firstInstance;

gl_VertexIndex (or gl_VertexID)

I'm doing a vkCmdDrawIndirect with a buffer of 8 * 3 vertices(8 triangles)

I have an array of 8 model matrices for each triangle, but I don't know how to get that array into the vertex shader and index it there for each triangle

I'd like to be able to index these in the vertex shader to apply the corresponding model matrix

I believe I could do something like this in the shader if I could somehow push the matrix array: model_array[gl_VertexIndex % 3]

transform_m4 size = 512
transform_m4[0] size = 64

push constants size limit = 128

So this would be a good fit for Storage Buffers I believe, since I would like this to scale to n number of models

Since descriptor sets are a way to get data into shaders
that's where we'll declare our storage buffer for the model matrices

The basic idea of using descriptor sets is to group your shader bindings by update frequency. So in your rendering loop you would have 1 descriptor set for per-camera params(e.g. projection and view), one for per-material params(e.g. different textures and values like specularity), one for per-object params(e.g. its transforms).


For each set n that is statically used by the VkPipeline ?:


---

VkBuffer buffers[] = { myBuffer, myBuffer, myBuffer, ... };
VkDeviceSize offsets[] = { 0x1000, 0x2000, 0x3000, ... };
vkCmdBindVertexBuffers(commandBuffer, 0, 3, buffers, offsets);

If we're talking about 2D, you can put your level into a Quadtree data structure and use that to very quickly cull parts of the level that are not on screen.

One example of such a restriction/constraint: Your 10,000 sprites are all the same base "model" (probably a quad) with only differences in how each "instance" is being affinely transformed.
In this case you can use instancing with a big Uniform Buffer Object to hold each instance's matrix in. This would require only one GL call to upload the UBO and only one draw call to draw all 10,000 instances at once.
You can then select the right matrix via gl_InstanceID in your shader.

Additional constraint: Every instance needs to have a separate texture or one of a few possible textures.
Solution: For this to work with instancing you could either use a texture atlas and per-instance texture coordinates, which you could also specify in the UBO, to select the right texture part for the right instance.
Or you could use array textures with each layer being a different texture and then encode the texture to be used inside the UBO as per-instance data (as a simple integer).
