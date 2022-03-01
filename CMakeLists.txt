# https://cristianadam.eu/20170709/speeding-up-cmake/

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(vkay VERSION 0.1.0)
FILE(GLOB SOURCES ./src/*.cpp)

set(THIRD_PARTY_LINK_DIRS
"./vendor/SDL2/lib/x64"
"./vendor/Vulkan/1.2.198.1/Lib"
"./vendor/assimp/lib"
)

set(INCLUDES
./src
# ./vendor/assimp/include
)

set (CFLAGS -FC -GR- -EHa- -EHsc -EHs -nologo /TP /Z7 /Zf /Zi /MP)

add_library(vkay STATIC ${SOURCES})
target_compile_options(vkay PRIVATE ${CFLAGS})
target_link_options(vkay PRIVATE -time)
target_link_directories(vkay PUBLIC ${THIRD_PARTY_LINK_DIRS})
target_link_libraries(vkay LINK_PUBLIC SDL2 SDL2main vulkan-1 assimp-vc142-mt)
target_include_directories(vkay PUBLIC ${INCLUDES})


# FILE(GLOB EXAMPLE_SOURCES ./examples/*.cpp)
add_executable(Instances ./examples/Instances.cpp)
target_compile_options(Instances PRIVATE ${CFLAGS})
target_link_options(Instances PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(Instances vkay)
target_include_directories(Instances PUBLIC ./src)

add_executable(Invaders ./examples/Invaders.cpp)
target_compile_options(Invaders PRIVATE ${CFLAGS})
target_link_options(Invaders PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(Invaders vkay)
target_include_directories(Invaders PUBLIC ./src)

add_executable(gltf ./examples/gltf.cpp)
target_compile_options(gltf PRIVATE ${CFLAGS})
target_compile_definitions(gltf PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_options(gltf PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(gltf vkay)
target_include_directories(gltf PUBLIC ./src)

set(OUT_DIR $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)

add_custom_command(TARGET vkay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL2/lib/x64/SDL2.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/SDL2.dll)

add_custom_command(TARGET vkay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/bin/assimp-vc142-mt.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/assimp-vc142-mt.dll)

# These will get problematic as shaders/ and assets/ get bigger since
# we are copying them each rebuild
# need a better solution at some point
# but for now, it's fine
add_custom_command(TARGET vkay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/shaders)

add_custom_command(TARGET vkay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/assets
)
