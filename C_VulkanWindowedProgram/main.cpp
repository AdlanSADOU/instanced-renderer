#include "Engine.h"

void testInit(Engine *engine);

int main()
{
	Engine engine;

	engine.Init_SDL();
	engine.Create_Window_SDL("[Project Name] Window", 0, 0, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	engine.Set_LayersAndInstanceExtensions();
	engine.Create_Instance();
	engine.Create_Surface_SDL();
	engine.Pick_PhysicalDevice();
	engine.Create_Device();
	engine.Create_Swapchain();
	engine.Setup_Engine();

    bool stillRunning = true;
    while(stillRunning) {

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
			
			const Uint8 *keystate = SDL_GetKeyboardState(NULL);
			keystate[SDL_SCANCODE_ESCAPE] ? stillRunning = false : 0;

            switch(event.type) {

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                break;
            }
        }

		// DO STUFF

    }

    // Clean up.
	vkDestroySwapchainKHR(engine.device, engine.swapchain, NULL);
    vkDestroySurfaceKHR(engine.instance, engine.surface, NULL);
	vkDestroyDevice(engine.device, NULL);
    SDL_DestroyWindow(engine.window);
    SDL_Quit();
    vkDestroyInstance(engine.instance, NULL);

    return 0;
}