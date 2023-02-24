// #define VKAY_DEBUG_ALLOCATIONS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <VkayBucket.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <Vkay.h>
#include <VkayCamera.h>

void UpdateAndRender();

bool  _mouse_left = false;
bool  _key_p      = false;
bool  _key_up = false, _key_down = false, _key_left = false, _key_right = false;
bool  _key_W = false, _key_A = false, _key_S = false, _key_D = false, _key_Q = false, _key_E = false, _key_R = false, _key_F = false;
float camera_x = 0, camera_y = 0, camera_z = 0;

float pos_x        = 0;
float pos_y        = 0;
float pos_z        = 0;
float camera_speed = 140.f;
float player_speed = 250.f;

const uint64_t MAX_DT_SAMPLES = 256;

float dt_samples[MAX_DT_SAMPLES] = {};
float dt_averaged                = 0;


Camera camera;

VkayRenderer vkr;
Quad         quad;

AllocatedImage render_target      = {};
VkImageView    render_target_view = {};
void          *pixels;

int main(int argc, char *argv[])
{
    VkayContextInit("Boids", 1920, 1080);
    VkayRendererInit(&vkr);

    VkayCameraCreate(&vkr, &camera);
    camera.m_projection = Camera::ORTHO;
    camera.m_position   = { 0, 0, 0 };


    bool res=VkayAllocateImageMemory(vkr.allocator, render_target.image, &render_target.allocation, VMA_MEMORY_USAGE_CPU_TO_GPU);
    if (!res)
        SDL_Log("allocation failed");

    VkImageCreateInfo ci_image = {};
    ci_image.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci_image.imageType         = VK_IMAGE_TYPE_2D;
    ci_image.format            = VK_FORMAT_R8G8B8A8_UNORM;
    ci_image.extent            = { (uint32_t)64, (uint32_t)64, 1 };
    ci_image.mipLevels         = 1;
    ci_image.arrayLayers       = 1;
    ci_image.samples           = VK_SAMPLE_COUNT_1_BIT;
    ci_image.usage             = VK_IMAGE_USAGE_SAMPLED_BIT;
    ci_image.tiling            = VK_IMAGE_TILING_OPTIMAL; // 0
    ci_image.sharingMode       = VK_SHARING_MODE_EXCLUSIVE; // 0
    ci_image.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED; // 0

    VmaAllocationCreateInfo ci_allocation = {};
    ci_allocation.usage                   = VMA_MEMORY_USAGE_GPU_TO_CPU;
    VKCHECK(vmaCreateImage(vkr.allocator, &ci_image, &ci_allocation, &render_target.image, &render_target.allocation, NULL));

    VKCHECK(vmaMapMemory(vkr.allocator, render_target.allocation, &pixels));

    // char *pixelBuffer = (char *)pixels;
    // pixelBuffer[0]    = 120;
    // pixelBuffer[1]    = 120;
    // pixelBuffer[2]    = 120;
    // pixelBuffer[3]    = 120;


    /////////////////////
    // Main loop
    UpdateAndRender();





    //////////////////////
    // Cleanup

    vkDeviceWaitIdle(vkr.device);
    VkayCameraDestroy(&vkr, &camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup();
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


            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT)
                    _mouse_left = true;
            }

            if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT)
                    _mouse_left = false;
            }

            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE) bQuit = true;

                if (key == SDLK_UP) _key_up = false;
                if (key == SDLK_DOWN) _key_down = false;
                if (key == SDLK_LEFT) _key_left = false;
                if (key == SDLK_RIGHT) _key_right = false;

                if (key == SDLK_w) _key_W = false;
                if (key == SDLK_a) _key_A = false;
                if (key == SDLK_s) _key_S = false;
                if (key == SDLK_d) _key_D = false;
                if (key == SDLK_q) _key_Q = false;
                if (key == SDLK_e) _key_E = false;
                if (key == SDLK_p) _key_p = false;

                if (key == SDLK_r) _key_R = false;
                if (key == SDLK_f) _key_F = false;
            }

            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) _key_up = true;
                if (key == SDLK_DOWN) _key_down = true;
                if (key == SDLK_LEFT) _key_left = true;
                if (key == SDLK_RIGHT) _key_right = true;

                if (key == SDLK_w) _key_W = true;
                if (key == SDLK_s) _key_S = true;
                if (key == SDLK_a) _key_A = true;
                if (key == SDLK_d) _key_D = true;
                if (key == SDLK_q) _key_Q = true;
                if (key == SDLK_e) _key_E = true;
                if (key == SDLK_p) _key_p = true;

                if (key == SDLK_r) _key_R = true;
                if (key == SDLK_f) _key_F = true;
            }
        }





        if (camera.m_projection == Camera::ORTHO) {
            if (_key_up) camera_y -= camera_speed * (float)dt_averaged;
            if (_key_down) camera_y += camera_speed * (float)dt_averaged;

            // if (_key_up) camera_z -= camera_speed * (float)dt_averaged;
            // if (_key_down) camera_z += camera_speed * (float)dt_averaged;
        } else {
            if (_key_R) camera_y -= camera_speed * (float)dt_averaged;
            if (_key_F) camera_y += camera_speed * (float)dt_averaged;
        }

        if (_key_left == true) {
            camera_x += camera_speed * (float)dt_averaged;
        }
        if (_key_right) camera_x -= camera_speed * (float)dt_averaged;

        if (_key_W) pos_y += player_speed * (float)dt_averaged;
        if (_key_S) pos_y -= player_speed * (float)dt_averaged;
        if (_key_A) pos_x -= player_speed * (float)dt_averaged;
        if (_key_D) pos_x += player_speed * (float)dt_averaged;
        // if (_key_Q) pos_z -= player_speed * (float)dt_averaged;
        // if (_key_E) pos_z += player_speed * (float)dt_averaged;
        if (_key_Q) pos_z -= .01f;
        if (_key_E) pos_z += .01f;





        //////////////////////////////////
        // Rendering
        if (!VkayRendererBeginCommandBuffer(&vkr)) return;

        camera.m_position.x += camera_x;
        camera.m_position.y += camera_y;
        camera.m_position.z += camera_z * .1f;

        // printf(" sprite: %.1f %.1f %.1f", s_pos->x, s_pos->y, s_pos->z);
        // printf(" camera: %.1f %.1f %.1f\n", camera.m_position.x, camera.m_position.y, camera.m_position.z);

        VkayCameraUpdate(&vkr, &camera, vkr.instanced_pipeline_layout);


        VkImageSubresourceLayers subresource;
        subresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.mipLevel       = 0;
        subresource.baseArrayLayer = 0;
        subresource.layerCount     = 1;

        VkOffset3D extent = { (int32_t)VkayGetContext()->window_extent.width, (int32_t)VkayGetContext()->window_extent.height, 0 };

        VkImageBlit region;
        region.srcSubresource = subresource;
        region.srcOffsets[0]  = { 0, 0, 0 };
        region.srcOffsets[1]  = extent;
        region.dstSubresource = subresource;
        region.dstOffsets[0]  = { 0, 0, 0 };
        region.dstOffsets[1]  = extent;

        vkCmdBlitImage(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, render_target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkr.swapchain.images[vkr.frame_idx_inflight], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererPresent(&vkr);

        pos_x = pos_y = pos_z = 0;
        camera_x = camera_y = camera_z = 0;

        //////////////////////////////////
        // DeltaTime
        uint64_t        end                = SDL_GetPerformanceCounter();
        static uint64_t idx                = 0;
        dt_samples[idx++ % MAX_DT_SAMPLES] = (end - start) / (float)SDL_GetPerformanceFrequency();
        float sum                          = 0;
        for (uint64_t i = 0; i < MAX_DT_SAMPLES; i++) {
            sum += dt_samples[i];
        }
        dt_averaged = sum / MAX_DT_SAMPLES;
    }
}
