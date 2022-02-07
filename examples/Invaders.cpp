#include <Vkay.h>
#include <VkayCamera.h>
#include <VkayTexture.h>
#include <VkaySprite.h>
#include <VkayInstances.h>

VkayContext  vkc;
VkayRenderer vkr;

bool   _bQuit       = false;
Color  _clear_color = Color(.33f, .33f, .33f, 1.f);
Camera _camera;

InstanceBucket _bucket;
vkay::Sprite   _spritePlayerShip;
vkay::Texture  _textureRedShip;
BaseMesh quad;
// SDL complains if char const *argv[]
int main(int argc, char *argv[])
{
    VkayContextInit(&vkc);
    VkayRendererInit(&vkc, &vkr);
    VkayCameraCreate(&vkr, &_camera);

    // VkayTextureCreate();
    _textureRedShip.Create("./assets/spaceships/red_01.png", &vkr, vkc);
    _spritePlayerShip.texture = &_textureRedShip;

    _bucket.AddSpriteInstance(_spritePlayerShip);
    VkayInstancesBucketUpload(&vkr, &_bucket, quad);

        while (!_bQuit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            SDL_Keycode key = e.key.keysym.sym;
            if (e.type == SDL_QUIT)
                _bQuit = true;
            if (e.type == SDL_KEYUP) {
                if (key == SDLK_ESCAPE)
                    _bQuit = true;
            }
        }

        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);

        VkayRendererClearColor(&vkr, _clear_color);
        VkayCameraUpdate(&vkr, &_camera, vkr.instanced_pipeline_layout);
        VkayDrawInstanceBucket(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, &vkr, &_bucket, BaseMesh());

        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        VkayRendererPresent(&vkr);
    }

    VkayCameraDestroy(&vkr, &_camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup(&vkc);

    return 0;
}
