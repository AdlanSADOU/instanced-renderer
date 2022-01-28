# https://cristianadam.eu/20170709/speeding-up-cmake/

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
if (UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -lpthread")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(vkay VERSION 0.1.0)
FILE(GLOB SOURCES ./src/*.cpp)

set(THIRD_PARTY_LINK_DIRS
"./vendor/SDL2/lib/x64"
"./vendor/Vulkan/1.2.198.1/Lib"
)

set(INCLUDES
./src
)

add_library(vkay STATIC ${SOURCES})
target_compile_options(vkay PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(vkay PRIVATE -time)
target_link_directories(vkay PUBLIC ${THIRD_PARTY_LINK_DIRS})
target_link_libraries(vkay SDL2 SDL2main vulkan-1)
target_include_directories(vkay PUBLIC ${INCLUDES})


# FILE(GLOB EXAMPLE_SOURCES ./examples/*.cpp)
add_executable(InstanceBuckets ./examples/InstanceBuckets.cpp)
target_compile_options(InstanceBuckets PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(InstanceBuckets PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(InstanceBuckets vkay)
target_include_directories(InstanceBuckets PUBLIC ./src)

add_executable(gltf ./examples/gltf.cpp)
target_compile_options(gltf PRIVATE -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
target_link_options(gltf PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(gltf vkay)
target_include_directories(gltf PUBLIC ./src)

set(OUT_DIR $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)

add_custom_command(TARGET vkay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL2/lib/x64/SDL2.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/SDL2.dll)

# These will get problematic as shaders/ and assets/ get bigger
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
