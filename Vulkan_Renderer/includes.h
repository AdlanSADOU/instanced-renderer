#pragma once

// Enable the WSI extensions
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED
#define STB_IMAGE_IMPLEMENTATION

#define API_VERSION VK_API_VERSION_1_0

#define WIDTH 1280
#define HEIGHT 720
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define MAX_FRAMES_IN_FLIGHT 2

#define VK_CHECK(status) \
	(status < VK_SUCCESS ? printf("something went wrong: vkRsult: %d!\nLine: %d in %s\n", status, __LINE__, __FILE__) : 0); \

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "stb_image.h"
#include "types.h"