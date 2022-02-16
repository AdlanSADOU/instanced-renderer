// there will be instanced and non instanced sprites

#if !defined(VKAY_SPRITE_H)
#define VKAY_SPRITE_H

#include <VkayTypes.h>

namespace vkay {
    struct Texture;

    struct Sprite
    {
        uint32_t texture_idx;
        Transform transform;
    };
}
#endif // VKAY_SPRITE_H
