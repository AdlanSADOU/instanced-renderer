// there will be instanced and non instanced sprites

#if !defined(VKAY_SPRITE)
#define VKAY_SPRITE

#include "VkayTypes.h"
#include "VkayTexture.h"
#include "VkayInstances.h"

namespace vky {
    class Sprite {
    private:
    public:
        Texture  *texture;
        Transform transform;
    };

} // namespace vky

#endif // VKAY_SPRITE
