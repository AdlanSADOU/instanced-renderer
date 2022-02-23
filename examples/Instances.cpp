// #define VKAY_DEBUG_ALLOCATIONS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <VkayInstancesBucket.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <Vkay.h>
#include <VkayCamera.h>

void UpdateAndRender();

bool  _key_p  = false;
bool  _key_up = false, _key_down = false, _key_left = false, _key_right = false;
bool  _key_W = false, _key_A = false, _key_S = false, _key_D = false, _key_Q = false, _key_E = false, _key_R = false, _key_F = false;
float camera_x = 0, camera_y = 0, camera_z = 0;

float pos_x        = 0;
float pos_y        = 0;
float pos_z        = 0;
float camera_speed = 140.f;
float player_speed = 250.f;

const uint64_t MAX_DT_SAMPLES = 256;

double dt_samples[MAX_DT_SAMPLES] = {};
double dt_averaged                = 0;

vkay::InstanceBucket instances;

vkay::Texture profile;
vkay::Texture itoldu;

Camera camera;

VkayContext  vkc;
VkayRenderer vkr;
Quad         quad;

int main(int argc, char *argv[])
{
    VkayContextInit(&vkc);
    VkayRendererInit(&vkc, &vkr);

    vkay::TextureCreate("./assets/texture.png", &profile, &vkr);
    vkay::TextureCreate("./assets/bjarn_itoldu.jpg", &itoldu, &vkr);

    VkayCameraCreate(&vkr, &camera);
    camera.m_projection = Camera::ORTHO;
    camera.m_position = {0,0,0};
    // InstanceData instance_data;

    const uint32_t ROW          = 3;
    const uint32_t COL          = 3;
    const int      spacing      = -5;
    uint32_t       SPRITE_COUNT = ROW * COL;
    SDL_Log("Sprites on screen: %d\n", SPRITE_COUNT);

    vkay::Sprite sprite = {};
    // Generate InstanceData for each sprite
    for (size_t i = 0, j = 0; i < SPRITE_COUNT; i++) {
        static float _x     = 0;
        static float _y     = 0;
        float        _scale = .1f;

        if (i > 0 && (i % ROW) == 0) j++;
        if (i == 0) _scale = 1.f;
        _x = (float)((profile.width + spacing) * (i % ROW) * _scale + 50);
        _y = (float)((profile.height + spacing) * j) * _scale + 50;

        sprite.transform.position = { _x-1.f, _y, .5f };
        if (i == 0) sprite.transform.position = { 0, 0, 0 };
        // if (i == 0) sprite.transform.position = { _x + vkc.window_extent.width / 2 - profile.width , -_y - vkc.window_extent.height / 2, 0.f };
        sprite.texture_idx     = profile.id;
        sprite.transform.scale = { profile.width, profile.height, 0 };
        sprite.transform.scale *= _scale;
        vkay::InstancesBucketAddSpriteInstance(&instances, &sprite);
    }

    vkay::InstancesBucketUpload(&vkr, &instances, quad.mesh);

    /////////////////////
    // Main loop
    UpdateAndRender();

    //////////////////////
    // Cleanup
    vkay::TextureDestroy(&profile, &vkr);
    vkay::TextureDestroy(&itoldu, &vkr);

    vkDeviceWaitIdle(vkr.device);
    vkay::InstancesDestroy(&vkr, &instances);
    VkayCameraDestroy(&vkr, &camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup(&vkc);
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
            // if (_key_up) camera_y -= camera_speed * (float)dt_averaged;
            // if (_key_down) camera_y += camera_speed * (float)dt_averaged;

            if (_key_up) camera_z -= camera_speed * (float)dt_averaged;
            if (_key_down) camera_z += camera_speed * (float)dt_averaged;
        } else
        {
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

        static int i = 0;
        if (_key_p) {
            vkay::InstancesDestroyInstance(&vkr, i++, &instances);
        }

        ///////////////////////////////
        // Compute dispatch
        vkWaitForFences(vkr.device, 1, &vkr.frames[0].compute_fence, true, SECONDS(1));
        vkResetFences(vkr.device, 1, &vkr.frames[0].compute_fence);
        vkResetCommandBuffer(vkr.frames[0].cmd_buffer_cmp, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(vkr.frames[0].cmd_buffer_cmp, &begin_info);
        vkCmdBindPipeline(vkr.frames[0].cmd_buffer_cmp, VK_PIPELINE_BIND_POINT_COMPUTE, vkr.compute_pipeline);
        vkCmdBindDescriptorSets(vkr.frames[0].cmd_buffer_cmp, VK_PIPELINE_BIND_POINT_COMPUTE, vkr.compute_pipeline_layout, 0, 1, vkr.compute_descriptor_sets.data(), 0, 0);
        vkCmdPushConstants(vkr.frames[0].cmd_buffer_cmp, vkr.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float_t), &pos_x);
        vkCmdDispatch(vkr.frames[0].cmd_buffer_cmp, (uint32_t)(instances.instance_data_array.size()), 1, 1);
        vkEndCommandBuffer(vkr.frames[0].cmd_buffer_cmp);

        VkSubmitInfo submit_info         = {};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount   = 0;
        submit_info.pWaitSemaphores      = NULL;
        submit_info.pWaitDstStageMask    = NULL;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &vkr.frames[0].cmd_buffer_cmp;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores    = NULL;
        vkQueueSubmit(vkr.compute_queue, 1, &submit_info, vkr.frames[0].compute_fence);

        //////////////////////////////////
        // Rendering
        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);

        glm::vec3 *s_pos = &((InstanceData *)instances.mapped_data_ptr)[0].pos;
        s_pos->x += pos_x;
        s_pos->y += pos_y;
        s_pos->z += pos_z * 1.f;

        camera.m_position.x += camera_x;
        camera.m_position.y += camera_y;
        camera.m_position.z += camera_z *.1f;

        printf(" sprite: %.1f %.1f %.1f", s_pos->x, s_pos->y, s_pos->z);
        printf(" camera: %.1f %.1f %.1f\n", camera.m_position.x, camera.m_position.y, camera.m_position.z);

        VkayCameraUpdate(&vkr, &camera, vkr.instanced_pipeline_layout);
        vkay::InstancesBucketDraw(VkayRendererGetCurrentFrameData(&vkr)->cmd_buffer_gfx, &vkr, &instances, quad.mesh);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        VkayRendererPresent(&vkr);

        //////////////////////////////////
        // DeltaTime
        pos_x = pos_y = pos_z = 0;
        camera_x = camera_y = camera_z     = 0;
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
