cmake_minimum_required(VERSION 3.16)

project(minecraft-cg C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    set(MINE_SOURCES mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c mine.rc)
    set(MINE_LIBRARIES glew32 freeglut opengl32 glu32 winmm)
elseif(APPLE)
    set(MINE_SOURCES mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
    set(MINE_LIBRARIES OpenGL::GL OpenGL::GLU GLUT::GLUT
        ${APPLICATION_SERVICES_FRAMEWORK})
else()
    set(MINE_SOURCES mine.c src/inventory.c src/crafting.c src/entities.c src/world.c src/chunks.c src/worldgen.c src/save.c src/effects.c src/audio.c src/player.c)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    find_package(GLEW REQUIRED)
    set(MINE_LIBRARIES OpenGL::GL OpenGL::GLU GLUT::GLUT GLEW::GLEW m)
endif()

function(add_minecraft_target target)
    add_executable(${target} ${MINE_SOURCES})
    target_link_libraries(${target} PRIVATE ${MINE_LIBRARIES})
    if(WIN32)
        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE FALSE)
    endif()
endfunction()

add_minecraft_target(mine)

add_minecraft_target(mine-matriz)
target_compile_definitions(mine-matriz PRIVATE DEMO_UI_MATRIX_ONLY=1)

add_minecraft_target(mine-barra)
target_compile_definitions(mine-barra PRIVATE DEMO_UI_FIXED_BAR=1)

add_minecraft_target(mine-mouse)
target_compile_definitions(mine-mouse PRIVATE DEMO_UI_RAW_MOUSE_Y=1)

add_minecraft_target(mine-pausa)
target_compile_definitions(mine-pausa PRIVATE DEMO_UI_PAUSE_CLOCK=1)

set_target_properties(mine-matriz mine-barra mine-mouse mine-pausa
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demonstracao"
)
