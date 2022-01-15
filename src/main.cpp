#include <vk_renderer.h>
#include <string>
#include <thread>

extern VulkanRenderer vkr;

bool  _up = false, _down = false, _left = false, _right = false;
bool  _W = false, _A = false, _S = false, _D = false, _Q = false, _E = false;
float camera_x = 0, camera_y = 0, camera_z = 0;

float pos_x = 0;
float pos_y = 0;
float pos_z = 0;

float camera_speed = 140.f;
float player_speed = 250.f;

const uint64_t MAX_DT_SAMPLES = 256;

double dt_samples[MAX_DT_SAMPLES] = {};
double dt_averaged                = 0;

void InitExamples();
void DestroyExamples();
void UpdateAndRender();
void ComputeExamples();
void DrawExamples(VkCommandBuffer *cmd_buffer, double dt);

extern int main(int argc, char *argv[])
{
    vk_Init();

    InitExamples();
    UpdateAndRender();
    DestroyExamples();

    vk_Cleanup();

    return 0;
}

void UpdateAndRender()
{
    SDL_Event e;
    bool      bQuit = false;


    while (!bQuit) {
        uint64_t start = SDL_GetPerformanceCounter();

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
                if (key == SDLK_q) _Q = false;
                if (key == SDLK_e) _E = false;
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
                if (key == SDLK_q) _Q = true;
                if (key == SDLK_e) _E = true;
            }
        }

        if (_up) camera_y -= camera_speed * (float)dt_averaged;
        if (_down) camera_y += camera_speed * (float)dt_averaged;
        if (_left) camera_x += camera_speed * (float)dt_averaged;
        if (_right) camera_x -= camera_speed * (float)dt_averaged;

        if (_W) pos_y += player_speed * (float)dt_averaged;
        if (_S) pos_y -= player_speed * (float)dt_averaged;
        if (_A) pos_x -= player_speed * (float)dt_averaged;
        if (_D) pos_x += player_speed * (float)dt_averaged;
        if (_Q) pos_z -= player_speed * (float)dt_averaged;
        if (_E) pos_z += player_speed * (float)dt_averaged;




        ComputeExamples();

        vk_BeginRenderPass();
        DrawExamples(&get_CurrentFrameData().cmd_buffer_gfx, dt_averaged);
        vk_EndRenderPass();

        pos_x = pos_y = pos_z = 0;


        static double acc = 0;
        if ((acc += dt_averaged) > 1) {
            SDL_SetWindowTitle(vkr.window, std::to_string(1 / dt_averaged).c_str());
            acc = 0;
        }
        // printf("seconds: %f\n", 1/dt_averaged);
        uint64_t end = SDL_GetPerformanceCounter();

        static uint64_t idx = 0;

        dt_samples[idx++ % MAX_DT_SAMPLES] = (end - start) / (double)SDL_GetPerformanceFrequency();

        double sum = 0;
        for (uint64_t i = 0; i < MAX_DT_SAMPLES; i++) {
            sum += dt_samples[i];
        }
        dt_averaged = sum / MAX_DT_SAMPLES;
    }
}
