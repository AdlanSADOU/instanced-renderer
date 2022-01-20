# https://cristianadam.eu/20170709/speeding-up-cmake/

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
if (UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -lpthread")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(vkay2d VERSION 0.1.0)
FILE(GLOB SOURCES ./src/*.cpp)

set(THIRD_PARTY_LINK_DIRS
# ./vendor/imgui
"./vendor/SDL2/lib/x64"
"./vendor/Vulkan/1.2.198.1/Lib"
)

set(THIRD_PARTY_INCLUDE_DIRS
./src
./vendor/Vulkan/1.2.198.1/Include
# ./vendor/SDL2/include
)

add_library(vkay2d STATIC ${SOURCES})
# target_compile_options(vkay2d PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(vkay2d PRIVATE -time)
target_link_directories(vkay2d PUBLIC ${THIRD_PARTY_LINK_DIRS})
target_link_libraries(vkay2d SDL2 SDL2main vulkan-1)
target_include_directories(vkay2d PUBLIC ${THIRD_PARTY_INCLUDE_DIRS})


# FILE(GLOB EXAMPLE_SOURCES ./examples/*.cpp)
add_executable(InstanceBuckets ./examples/InstanceBuckets.cpp)
target_compile_options(InstanceBuckets PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(InstanceBuckets PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(InstanceBuckets vkay2d)
target_include_directories(InstanceBuckets PUBLIC ./src)

add_executable(gltf ./examples/gltf.cpp)
target_compile_options(gltf PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(gltf PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(gltf vkay2d)
target_include_directories(gltf PUBLIC ./src)

add_custom_command(
        TARGET vkay2d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL2/lib/x64/SDL2.dll ${CMAKE_CURRENT_BINARY_DIR}/Debug)

add_custom_command(TARGET vkay2d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_BINARY_DIR}/Debug/shaders)

add_custom_command(TARGET vkay2d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/Debug/assets)
