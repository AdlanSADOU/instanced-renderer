#include <Vkay.h>
#include <VkayCamera.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <VkayInstancesBucket.h>

// VkayContext  vkc;
VkayRenderer vkr;

bool   _bQuit       = false;
Color  _clear_color = Color(.22f, .22f, .22f, 0.f);
Camera _camera;

struct Controls
{
    bool key_p  = false;
    bool key_up = false, key_down = false, key_left = false, key_right = false;
    bool key_W = false, key_A = false, key_S = false, key_D = false, key_Q = false, key_E = false, key_R = false, key_F = false;
} ctrls;

vkay::InstanceBucket _bucket;
vkay::Sprite         _spritePlayerShip;
vkay::Texture        _textureRedShip;

int main(int argc, char *argv[])
{
    VkayContextInit("Vkay - Invaders", 1280, 720);
    VkayRendererInit(&vkr);
    VkayCameraCreate(&vkr, &_camera);
    _camera.m_projection = Camera::ORTHO;
    _camera.m_position   = { 0, 0, 0 };

    vkay::TextureCreate("./assets/spaceships/red_01.png", &_textureRedShip, &vkr);

    _spritePlayerShip.texture_idx        = _textureRedShip.id;
    _spritePlayerShip.transform.position = { 55, 40, 0 };
    _spritePlayerShip.transform.scale    = { _textureRedShip.width, _textureRedShip.height, 1 };

    vkay::InstancesBucketAddSpriteInstance(&_bucket, &_spritePlayerShip);
    vkay::InstancesBucketUpload(&vkr, &_bucket, Quad().mesh);

    while (!_bQuit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int width = 0, height = 0;
                    // SDL_GetWindowSize(VkayGetContext()->window, (int *)&VkayGetContext()->window_extent.width, (int *)&VkayGetContext()->window_extent.height);
                }
            }
 
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT)
                _bQuit = true;
            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE)
                    _bQuit = true;

                if (key == SDLK_UP) ctrls.key_up = false;
                if (key == SDLK_DOWN) ctrls.key_down = false;
                if (key == SDLK_LEFT) ctrls.key_left = false;
                if (key == SDLK_RIGHT) ctrls.key_right = false;

                if (key == SDLK_w) ctrls.key_W = false;
                if (key == SDLK_a) ctrls.key_A = false;
                if (key == SDLK_s) ctrls.key_S = false;
                if (key == SDLK_d) ctrls.key_D = false;
                if (key == SDLK_q) ctrls.key_Q = false;
                if (key == SDLK_e) ctrls.key_E = false;
                if (key == SDLK_p) ctrls.key_p = false;

                if (key == SDLK_r) ctrls.key_R = false;
                if (key == SDLK_f) ctrls.key_F = false;
            }

            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) ctrls.key_up = true;
                if (key == SDLK_DOWN) ctrls.key_down = true;
                if (key == SDLK_LEFT) ctrls.key_left = true;
                if (key == SDLK_RIGHT) ctrls.key_right = true;

                if (key == SDLK_w) ctrls.key_W = true;
                if (key == SDLK_s) ctrls.key_S = true;
                if (key == SDLK_a) ctrls.key_A = true;
                if (key == SDLK_d) ctrls.key_D = true;
                if (key == SDLK_q) ctrls.key_Q = true;
                if (key == SDLK_e) ctrls.key_E = true;
                if (key == SDLK_p) ctrls.key_p = true;

                if (key == SDLK_r) ctrls.key_R = true;
                if (key == SDLK_f) ctrls.key_F = true;
            }
        }

        glm::vec3 *s_pos = &((InstanceData *)_bucket.mapped_data_ptr)[0].pos;

        float shipX = 0, shipY = 0, shipZ = 0;
        float cameraX = 0, cameraY = 0, cameraZ = 0;

        if (ctrls.key_W) shipY += .5f;
        if (ctrls.key_S) shipY -= .5f;
        if (ctrls.key_A) shipX -= .5f;
        if (ctrls.key_D) shipX += .5f;
        if (ctrls.key_Q) shipZ -= .5f;
        if (ctrls.key_E) shipZ += .5f;

        if (ctrls.key_up) cameraY += .5f;
        if (ctrls.key_down) cameraY -= .5f;
        if (ctrls.key_left) cameraX -= .5f;
        if (ctrls.key_right) cameraX += .5f;
        if (ctrls.key_R) cameraZ -= .5f;
        if (ctrls.key_F) cameraZ += .5f;

        s_pos->x += shipX * 1.f;
        s_pos->y += shipY * 1.f;
        s_pos->z += shipZ * .1f;

        _camera.m_position.x += cameraX * 1.f;
        _camera.m_position.y += cameraY * 1.f;
        _camera.m_position.z += cameraZ * .1f;

        // printf(" sprite: %f.1 %f.1 %f.1", s_pos->x, s_pos->y, s_pos->z);
        // printf(" camera: %f.1 %f.1 %f.1\n", _camera.m_position.x, _camera.m_position.y, _camera.m_position.z);

        // todo(ad): @urgent this if statent must go
        if (!VkayRendererBeginCommandBuffer(&vkr))
            continue;

        VkayRendererClearColor(&vkr, _clear_color);
        VkayCameraUpdate(&vkr, &_camera, vkr.instanced_pipeline_layout);
        vkay::InstancesBucketDraw(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, &vkr, &_bucket, Quad().mesh);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererPresent(&vkr);
    }

    vkDeviceWaitIdle(vkr.device);
    vkay::InstancesDestroy(&vkr, &_bucket);
    vkay::TextureDestroy(&_textureRedShip, &vkr);
    VkayCameraDestroy(&vkr, &_camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup();

    return 0;
}
