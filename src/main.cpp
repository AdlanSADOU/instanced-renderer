#include <vk_renderer.h>

int main(int argc, char *argv[])
{
    VulkanRenderer engine;

    engine.init();
    engine.run();
    engine.cleanup();

    return 0;
}
