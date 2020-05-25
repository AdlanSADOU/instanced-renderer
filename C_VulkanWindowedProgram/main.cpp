#include "Engine.h"

int main()
{
	Engine engine = Engine();
	engine.SetupEngine();

    // Poll for user input.
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
                // Do nothing.
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