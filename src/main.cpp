#include <vk_renderer.h>

int main(int argc, char *argv[])
{
    vk_Init();
    vk_Render();
    UpdateAndRender();
    vk_Cleanup();

    return 0;
}
