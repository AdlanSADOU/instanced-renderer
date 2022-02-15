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

vkay::InstanceBucket _bucket;
vkay::Sprite   _spritePlayerShip;
vkay::Texture  _textureRedShip;
// BaseMesh quad = BaseMesh();
// SDL complains if char const *argv[]
int main(int argc, char *argv[])
{
    VkayContextInit(&vkc);
    VkayRendererInit(&vkc, &vkr);
    VkayCameraCreate(&vkr, &_camera);

    // VkayTextureCreate();
    vkay::TextureCreate("./assets/spaceships/red_01.png", &vkr, vkc, &_textureRedShip);
    _spritePlayerShip.texture = &_textureRedShip;
    _spritePlayerShip.transform.scale = {_textureRedShip.width, _textureRedShip.height, 1};

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
            }
        }

        VkayRendererBeginCommandBuffer(&vkr);
        VkayRendererBeginRenderPass(&vkr);

        VkayRendererClearColor(&vkr, _clear_color);
        VkayCameraUpdate(&vkr, &_camera, vkr.instanced_pipeline_layout);
        vkay::InstancesBucketDraw(vkr.frames[vkr.frame_idx_inflight].cmd_buffer_gfx, &vkr, &_bucket, Quad().mesh);

        VkayRendererEndRenderPass(&vkr);
        VkayRendererEndCommandBuffer(&vkr);
        VkayRendererPresent(&vkr);
    }

    vkay::InstancesDestroy(&vkr, &_bucket);
    vkay::TextureDestroy(&vkr, vkc, &_textureRedShip);
    VkayCameraDestroy(&vkr, &_camera);
    VkayRendererCleanup(&vkr);
    VkayContextCleanup(&vkc);

    return 0;
}
