#include <stdio.h>
#include <vulkan/vulkan.h>


#define VKCHECK(x)                                      \
    do {                                                \
        VkResult err = x;                               \
        if (err) {                                      \
            printf("Detected Vulkan error: %d\n", err); \
            abort();                                    \
        }                                               \
    } while (0);

int cvkayCreateDevice(VkInstance *pInstance)
{
    printf("Creating Instance...\n");

    VkApplicationInfo applicationInfo  = { 0 };
    applicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext              = NULL;
    applicationInfo.pApplicationName   = "cvkay";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pEngineName        = "cvkay engine";
    applicationInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ciInstance = { 0 };
    ciInstance.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ciInstance.pNext                = NULL;
    ciInstance.flags                = 0;
    ciInstance.pApplicationInfo     = &applicationInfo;
    ciInstance.enabledLayerCount;
    ciInstance.ppEnabledLayerNames;
    ciInstance.enabledExtensionCount;
    ciInstance.ppEnabledExtensionNames;
    VKCHECK((vkCreateInstance(&ciInstance, NULL, pInstance)));

    return 1;
}