#include <Vkay.h>
#include <VkayCamera.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <VkayBucket.h>

// VkayContext  vkc;
static VkayRenderer vkr;

static bool   _bQuit       = false;
static Color  _clear_color = Color(.22f, .22f, .22f, 0.f);
static Camera _camera;

struct Keys
{
    bool key_p  = false;
    bool key_up = false, key_down = false, key_left = false, key_right = false;
    bool key_W = false, key_A = false, key_S = false, key_D = false, key_Q = false, key_E = false, key_R = false, key_F = false;
} keys;

static vkay::InstanceBucket _bucket;
static vkay::Sprite         _sprite_player_ship;
static vkay::Texture        _texture_red_ship;

int main(int argc, char *argv[])
{
    VkayContextInit("Vkay - Invaders", 1280, 720);
    VkayRendererInit(&vkr);
    VkayCameraCreate(&vkr, &_camera);
    _camera.m_projection = Camera::ORTHO;
    _camera.m_position   = { 0, 0, 0 };

    vkay::TextureCreate("./assets/spaceships/red_01.png", &_texture_red_ship, &vkr);

    _sprite_player_ship.texture_idx        = _texture_red_ship.id;
    _sprite_player_ship.transform.position = { 55, 40, 0 };
    _sprite_player_ship.transform.scale    = { _texture_red_ship.width, _texture_red_ship.height, 1 };

    vkay::BucketAddSpriteInstance(&_bucket, &_sprite_player_ship);
    vkay::BucketUpload(&vkr, &_bucket, Quad().mesh);

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

                if (key == SDLK_UP) keys.key_up = false;
                if (key == SDLK_DOWN) keys.key_down = false;
                if (key == SDLK_LEFT) keys.key_left = false;
                if (key == SDLK_RIGHT) keys.key_right = false;

                if (key == SDLK_w) keys.key_W = false;
                if (key == SDLK_a) keys.key_A = false;
                if (key == SDLK_s) keys.key_S = false;
                if (key == SDLK_d) keys.key_D = false;
                if (key == SDLK_q) keys.key_Q = false;
                if (key == SDLK_e) keys.key_E = false;
                if (key == SDLK_p) keys.key_p = false;

                if (key == SDLK_r) keys.key_R = false;
                if (key == SDLK_f) keys.key_F = false;
            }

            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP) keys.key_up = true;
                if (key == SDLK_DOWN) keys.key_down = true;
                if (key == SDLK_LEFT) keys.key_left = true;
                if (key == SDLK_RIGHT) keys.key_right = true;

                if (key == SDLK_w) keys.key_W = true;
                if (key == SDLK_s) keys.key_S = true;
                if (key == SDLK_a) keys.key_A = true;
                if (key == SDLK_d) keys.key_D = true;
                if (key == SDLK_q) keys.key_Q = true;
                if (key == SDLK_e) keys.key_E = true;
                if (key == SDLK_p) keys.key_p = true;

                if (key == SDLK_r) keys.key_R = true;
                if (key == SDLK_f) keys.key_F = true;
            }
        }

        glm::vec3 *s_pos = &((InstanceData *)_bucket.mapped_data_ptr)[0].pos;

        float shipX = 0, shipY = 0, shipZ = 0;
        float cameraX = 0, cameraY = 0, cameraZ = 0;

        if (keys.key_W) shipY += .5f;
        if (keys.key_S) shipY -= .5f;
        if (keys.key_A) shipX -= .5f;
        if (keys.key_D) shipX += .5f;
        if (keys.key_Q) shipZ -= .5f;
        if (keys.key_E) shipZ += .5f;

        if (keys.key_up) cameraY += .5f;
        if (keys.key_down) cameraY -= .5f;
        if (keys.key_left) cameraX -= .5f;
        if (keys.key_right) cameraX += .5f;
        if (keys.key_R) cameraZ -= .5f;
        if (keys.key_F) cameraZ += .5f;

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
        vkay::BucketDraw(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, &vkr, &_bucket, Quad().mesh);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererPresent(&vkr);
    }

    vkDeviceWaitIdle(vkr.device);
    vkay::BucketDestroy(&vkr, &_bucket);
    vkay::TextureDestroy(&_texture_red_ship, &vkr);
    VkayCameraDestroy(&vkr, &_camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup();

    return 0;
}
