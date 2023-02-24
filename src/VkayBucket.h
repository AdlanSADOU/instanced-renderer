/*
    This file contains the implementation of IntanceBucket
    An InstanceBucket is a data structure that holds data about instances of a same base mesh
    which allows you to draw them all with only one draw call very efficiently
    using instanced rendering.

    i.e: you could add many sprites with different Texture(s) and Transform(s)
    and draw them all at once.

    This is particularly useful for something like a particle system.
*/

#if !defined(VKAY_INSTANCES)
#define VKAY_INSTANCES

struct InstanceData;
struct VkayBuffer;

#include <VkayTypes.h>
#include <vector>

struct VkayRenderer;

namespace vkay {
    struct Sprite;

    struct InstanceBucket
    {
        void                     *mapped_data_ptr     = {};
        std::vector<InstanceData> instance_data_array = {};
        VkayBuffer                instance_buffer     = {};

        VkayBuffer mesh_buffer = {};
    };

    void BucketAddSpriteInstance(InstanceBucket *instance_bucket, Sprite *sprite);
    void BucketSetBaseMesh();
    bool BucketUpload(VkayRenderer *vkr, InstanceBucket *bucket, BaseMesh base_mesh);
    void BucketDestroyInstance(VkayRenderer *vkr, uint32_t instance_index, InstanceBucket *bucket);
    void BucketDraw(VkCommandBuffer cmd_buffer, VkayRenderer *vkr, InstanceBucket *bucket, BaseMesh base_mesh);
    void BucketDestroy(VkayRenderer *vkr, InstanceBucket *bucket);
}

#endif // VKAY_INSTANCES
