#include <vk_renderer.h>

bool       _up = false, _down = false, _left = false, _right = false;
bool       _W = false, _S = false, _A = false, _D = false;
float      camera_x = 0, camera_y = 0, camera_z = 0;

void UpdateAndRender();

int main(int argc, char *argv[])
{
    vk_Init();
    vk_BeginPass();
    UpdateAndRender();
    vk_Cleanup();

    return 0;
}

void UpdateAndRender()
{
    SDL_Event e;
    bool      bQuit = false;

    while (!bQuit) {

        while (SDL_PollEvent(&e) != 0) {
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT) bQuit = true;

            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE) bQuit = true;

                if (key == SDLK_UP) _up = false;
                if (key == SDLK_DOWN) _down = false;
                if (key == SDLK_LEFT) _left = false;
                if (key == SDLK_RIGHT) _right = false;

                if (key == SDLK_w) _W = false;
                if (key == SDLK_s) _S = false;
                if (key == SDLK_a) _A = false;
                if (key == SDLK_d) _D = false;
            }
            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) _up = true;
                if (key == SDLK_DOWN) _down = true;
                if (key == SDLK_LEFT) _left = true;
                if (key == SDLK_RIGHT) _right = true;

                if (key == SDLK_w) _W = true;
                if (key == SDLK_s) _S = true;
                if (key == SDLK_a) _A = true;
                if (key == SDLK_d) _D = true;
            }
        }

        if (_up) camera_z += .1f;
        if (_down) camera_z -= .1f;
        if (_left) camera_x -= .1f;
        if (_right) camera_x += .1f;

        static float posX = 0;
        static float posY = 0;

        if (_W) posY += .3f;
        if (_S) posY -= .3f;
        if (_A) posX -= .3f;
        if (_D) posX += .3f;

        // static RenderObject *monkey = get_Renderable("monkey");

        // monkey->transformMatrix = glm::translate(monkey->transformMatrix, glm::vec3(posX, posY, 0));

        // posX = posY = 0;

        vk_BeginPass();
    }
}