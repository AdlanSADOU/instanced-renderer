#if !defined(CVKAY_H)
#define CVKAY_H
#include <vulkan/vulkan.h>

#define ARRCOUNT(arr) (sizeof(arr) / sizeof(arr[0]))

// @delete
typedef struct CVkayDevice
{
    VkInstance instance;
    VkDevice   device;
} cvkayDevice;

#define VKCHECK(x)                                      \
    do {                                                \
        VkResult err = x;                               \
        if (err) {                                      \
            printf("Detected Vulkan error: %s\n", err); \
            abort();                                    \
        }                                               \
    } while (0);

int cvkayCreateDevice();

#endif // CVKAY_H
