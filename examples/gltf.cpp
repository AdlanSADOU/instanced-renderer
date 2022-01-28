/**
 * Things to investigate:
 * - https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#_description_39
 * - https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/performance/layout_transitions/layout_transitions_tutorial.html
 * -https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#synchronization-image-memory-barriers
 *
 * The goal of this sample is to directly write into the acquired swapchain images
 * through a compute shader.
 * This might not work on all platforms as VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT has to be supported
 * we must check through vkGetPhysicalDeviceSurfaceCapabilitiesKHR
 * It will likely work tho even without support for the above flag, but validation layer will throw errors
 *
 * Also when creation the swapchain we have to set VK_IMAGE_USAGE_STORAGE_BIT
 * And without a RenderPass we may need to transition imageLayout manually
 */

#include <VkayRenderer.h>
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

void UpdateAndRender();

bool  _key_r  = false;
bool  _key_up = false, _key_down = false, _key_left = false, _key_right = false;
bool  _key_W = false, _key_A = false, _key_S = false, _key_D = false, _key_Q = false, _key_E = false;
float camera_x = 0, camera_y = 0, camera_z = 0;

float pos_x        = 0;
float pos_y        = 0;
float pos_z        = 0;
float camera_speed = 140.f;
float player_speed = 250.f;

const uint64_t MAX_DT_SAMPLES = 256;

double dt_samples[MAX_DT_SAMPLES] = {};
double dt_averaged                = 0;

VkayRenderer vkr;
cgltf_data  *data;
VkayBuffer ibo = {};
VkayBuffer vbo = {};



int main(int argc, char *argv[])
{
    VkayRendererInit(&vkr);


    cgltf_options options = {};
    cgltf_parse_file(&options, "./assets/3D/khronos_logo/scene.gltf", &data);

    /////////////////
    // Index Buffer
    uint32_t indices_size = (uint32_t)data->meshes->primitives->indices->count * sizeof(uint32_t);
    VK_CHECK(VkayCreateBuffer(&ibo, vkr.allocator, indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY));

    VkayBuffer staging_buffer = {};
    VK_CHECK(VkayCreateBuffer(&staging_buffer, vkr.allocator, indices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));
    VK_CHECK(VkayMapMemcpyMemory(data->meshes->primitives->indices, indices_size, vkr.allocator, staging_buffer.allocation));

    VkayBeginCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VkBufferCopy region = {};
    region.size = indices_size;
    vkCmdCopyBuffer(vkr.frames[0].cmd_buffer_gfx, staging_buffer.buffer, ibo.buffer, 1, &region);
    VkayEndCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VK_CHECK(VkayQueueSumbit(vkr.graphics_queue, &vkr.frames[0].cmd_buffer_gfx));
    vkDeviceWaitIdle(vkr.device);

    //////////////////
    // Vertex Buffer
    cgltf_data* vertex_data = {};

    cgltf_result res = cgltf_load_buffers(&options, data, "./assets/3D/khronos_logo/scene.bin");
    uint32_t size = (uint32_t)data->buffers[0].size;
    VK_CHECK(VkayCreateBuffer(&ibo, vkr.allocator, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY));

    VkayBuffer vertex_staging_buffer = {};
    VK_CHECK(VkayCreateBuffer(&vertex_staging_buffer, vkr.allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));
    VK_CHECK(VkayMapMemcpyMemory(data->buffers[0].data, size, vkr.allocator, vertex_staging_buffer.allocation));

    VkayBeginCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VkBufferCopy vertex_buffer_region = {};
    vertex_buffer_region.size = size;
    vkCmdCopyBuffer(vkr.frames[0].cmd_buffer_gfx, vertex_staging_buffer.buffer, ibo.buffer, 1, &vertex_buffer_region);
    VkayEndCommandBuffer(vkr.frames[0].cmd_buffer_gfx);
    VK_CHECK(VkayQueueSumbit(vkr.graphics_queue, &vkr.frames[0].cmd_buffer_gfx));
    vkDeviceWaitIdle(vkr.device);

    /////////////////////
    // Main loop
    UpdateAndRender();

    //////////////////////
    // Cleanup
    vkDeviceWaitIdle(vkr.device);
    VkayRendererCleanup(&vkr);

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
                if (key == SDLK_r) _key_r = false;
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
                if (key == SDLK_r) _key_r = true;
            }
        }

        if (_key_up) camera_y -= camera_speed * (float)dt_averaged;
        if (_key_down) camera_y += camera_speed * (float)dt_averaged;
        if (_key_left) camera_x += camera_speed * (float)dt_averaged;
        if (_key_right) camera_x -= camera_speed * (float)dt_averaged;

        if (_key_W) pos_y += player_speed * (float)dt_averaged;
        if (_key_S) pos_y -= player_speed * (float)dt_averaged;
        if (_key_A) pos_x -= player_speed * (float)dt_averaged;
        if (_key_D) pos_x += player_speed * (float)dt_averaged;
        if (_key_Q) pos_z -= player_speed * (float)dt_averaged;
        if (_key_E) pos_z += player_speed * (float)dt_averaged;



        ///////////////////////////////
        // Compute dispatch

        //////////////////////////////////


        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);
        VkCommandBuffer cmd_buffer_gfx = vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx;
        vkCmdBindPipeline(cmd_buffer_gfx, VK_PIPELINE_BIND_POINT_GRAPHICS, vkr.default_pipeline);
         vkCmdBindIndexBuffer(cmd_buffer_gfx, ibo.buffer, 0, VK_INDEX_TYPE_UINT32);
         vkCmdBindVertexBuffers(cmd_buffer_gfx, 0, 1, &vbo.buffer, 0);
         vkCmdDrawIndexed(cmd_buffer_gfx, data->meshes->primitives->indices->count, 1, 0, 0, 0);
        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        // DeltaTime
        pos_x = pos_y = pos_z              = 0;
        uint64_t        end                = SDL_GetPerformanceCounter();
        static uint64_t idx                = 0;
        dt_samples[idx++ % MAX_DT_SAMPLES] = (end - start) / (double)SDL_GetPerformanceFrequency();
        double sum                         = 0;
        for (uint64_t i = 0; i < MAX_DT_SAMPLES; i++) {
            sum += dt_samples[i];
        }
        dt_averaged = sum / MAX_DT_SAMPLES;
    }
}
