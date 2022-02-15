#include <vector>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "VkayTypes.h"

struct Texture;

struct Mesh
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture>  textures;
    glm::mat4             transformMatrix = {};
    Material             *material        = {};
    char                 *filepath;
};
