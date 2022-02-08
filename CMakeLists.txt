# https://cristianadam.eu/20170709/speeding-up-cmake/

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(vkay VERSION 0.1.0)

set(THIRD_PARTY_LINK_DIRS
"./vendor/SDL2/lib/x64"
"./vendor/Vulkan/1.2.198.1/Lib"
"./vendor/assimp/lib"
)

set(INCLUDES
./src
./vendor/assimp/include
)

# ----------------- C Project -------------------------------
set (CPP_FLAGS -FC -GR- -EHa- -EHsc -EHs -Tc -nologo -Za -Zi -Zf /MP)
FILE(GLOB C_SOURCES ./c_src/*.c)
add_library(cvkay STATIC ${C_SOURCES})
target_compile_options(cvkay PRIVATE ${CFLAGS})
target_link_options(cvkay PRIVATE -time)
target_link_directories(cvkay PUBLIC ${THIRD_PARTY_LINK_DIRS})
target_link_libraries(cvkay SDL2 SDL2main vulkan-1)
target_include_directories(cvkay PUBLIC ${INCLUDES})

# FILE(GLOB EXAMPLE_SOURCES ./examples/*.cpp)
add_executable(ctest ./examples/ctest.c)
target_compile_options(ctest PRIVATE ${CFLAGS})
target_link_options(ctest PRIVATE -SUBSYSTEM:CONSOLE -time)
target_link_libraries(ctest cvkay)
target_include_directories(ctest PUBLIC ./src)
#//////////////////////////////////////////////////////////////




# set (CPP_FLAGS -FC -GR- -EHa- -EHsc -EHs -nologo -Zi -Zf /MP)
# FILE(GLOB SOURCES ./src/*.cpp)
# add_library(vkay STATIC ${SOURCES})
# target_compile_options(vkay PRIVATE ${CPP_FLAGS})
# target_link_options(vkay PRIVATE -time)
# target_link_directories(vkay PUBLIC ${THIRD_PARTY_LINK_DIRS})
# target_link_libraries(vkay SDL2 SDL2main vulkan-1 assimp-vc142-mt)
# target_include_directories(vkay PUBLIC ${INCLUDES})

# add_executable(InstanceBuckets ./examples/InstanceBuckets.cpp)
# target_compile_options(InstanceBuckets PRIVATE ${CPP_FLAGS})
# target_link_options(InstanceBuckets PRIVATE -SUBSYSTEM:CONSOLE -time)
# target_link_libraries(InstanceBuckets vkay)
# target_include_directories(InstanceBuckets PUBLIC ./src)

# add_executable(Invaders ./examples/Invaders.cpp)
# target_compile_options(Invaders PRIVATE ${CPP_FLAGS})
# target_link_options(Invaders PRIVATE -SUBSYSTEM:CONSOLE -time)
# target_link_libraries(Invaders vkay)
# target_include_directories(Invaders PUBLIC ./src)

# add_executable(gltf ./examples/gltf.cpp)
# target_compile_options(gltf PRIVATE ${CPP_FLAGS})
# target_compile_definitions(gltf PRIVATE _CRT_SECURE_NO_WARNINGS)
# target_link_options(gltf PRIVATE -SUBSYSTEM:CONSOLE -time)
# target_link_libraries(gltf vkay)
# target_include_directories(gltf PUBLIC ./src)

set(OUT_DIR $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)

add_custom_command(TARGET cvkay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL2/lib/x64/SDL2.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/SDL2.dll

        )

add_custom_command(TARGET cvkay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/bin/assimp-vc142-mt.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/assimp-vc142-mt.dll)

# These will get problematic as shaders/ and assets/ get bigger
# need a better solution at some point
# but for now, it's fine
add_custom_command(TARGET cvkay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/shaders)

add_custom_command(TARGET cvkay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/${OUT_DIR}/assets
)
