#include <Vkay.h>
#include <VkayCamera.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <VkayInstancesBucket.h>

VkayContext  vkc;
VkayRenderer vkr;

bool   _bQuit       = false;
Color  _clear_color = Color(.22f, .22f, .22f, 0.f);
Camera _camera;

struct Controls
{
    bool up    = false;
    bool down  = false;
    bool left  = false;
    bool right = false;
} ctrls;

vkay::InstanceBucket _bucket;
vkay::Sprite         _spritePlayerShip;
vkay::Texture        _textureRedShip;
// BaseMesh quad = BaseMesh();
// SDL complains if char const *argv[]
int main(int argc, char *argv[])
{
    VkayContextInit(&vkc);
    VkayRendererInit(&vkc, &vkr);
    VkayCameraCreate(&vkr, &_camera);
    _camera.m_projection = Camera::ORTHO;
    _camera.m_position   = { 0, 0, 10.f };

    vkay::TextureCreate("./assets/spaceships/red_01.png", &_textureRedShip, &vkr);
    _spritePlayerShip.texture_idx        = _textureRedShip.id;
    _spritePlayerShip.transform.position = { _textureRedShip.width/2, _textureRedShip.height/2, 0 };

    // todo(ad): this scales up the quad to the size of the texture
    _spritePlayerShip.transform.scale    = { _textureRedShip.width, _textureRedShip.height, 1 };

    vkay::InstancesBucketAddSpriteInstance(&_bucket, &_spritePlayerShip);
    vkay::InstancesBucketUpload(&vkr, &_bucket, Quad().mesh);

    while (!_bQuit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT)
                _bQuit = true;
            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE)
                    _bQuit = true;
                if (key == SDLK_UP)
                    ctrls.up = false;
                if (key == SDLK_DOWN)
                    ctrls.down = false;
                if (key == SDLK_LEFT)
                    ctrls.left = false;
                if (key == SDLK_RIGHT)
                    ctrls.right = false;
            }
            if (e.type == SDL_KEYDOWN) {
                if (key == SDLK_UP)
                    ctrls.up = true;
                if (key == SDLK_DOWN)
                    ctrls.down = true;
                if (key == SDLK_LEFT)
                    ctrls.left = true;
                if (key == SDLK_RIGHT)
                    ctrls.right = true;
            }
        }

        if (ctrls.up)
            ((InstanceData *)_bucket.mapped_data_ptr)[0].pos.y += 1.f;
        if (ctrls.down)
            ((InstanceData *)_bucket.mapped_data_ptr)[0].pos.y -= 1.f;
        if (ctrls.left)
            ((InstanceData *)_bucket.mapped_data_ptr)[0].pos.x -= 1.f;
        if (ctrls.right)
            ((InstanceData *)_bucket.mapped_data_ptr)[0].pos.x += 1.f;

        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);

        VkayRendererClearColor(&vkr, _clear_color);
        VkayCameraUpdate(&vkr, &_camera, vkr.instanced_pipeline_layout);
        vkay::InstancesBucketDraw(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, &vkr, &_bucket, Quad().mesh);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        VkayRendererPresent(&vkr);
    }

    vkDeviceWaitIdle(vkr.device);
    vkay::InstancesDestroy(&vkr, &_bucket);
    vkay::TextureDestroy(&_textureRedShip, &vkr);
    VkayCameraDestroy(&vkr, &_camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup(&vkc);

    return 0;
}
