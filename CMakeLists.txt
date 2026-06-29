cmake_minimum_required(VERSION 3.21)
project(extropian-render VERSION 0.1.0 LANGUAGES CXX
    DESCRIPTION "Extropian Render: 3D rendering with OpenGL backend"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ext-core REQUIRED)
find_package(OpenGL REQUIRED)

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.18)
    FetchContent_MakeAvailable(SDL3)
endif()

add_library(exd-render STATIC
    # OpenGL backend
    src/backend/opengl/gl_mesh.cpp
    src/backend/opengl/gl_shader.cpp
    src/backend/opengl/gl_texture.cpp
    src/backend/opengl/gl_renderer.cpp
    src/backend/opengl/gl_particle_volume.cpp

    # Null backend
    src/backend/null/null_renderer.cpp

    # Sci-vis stubs (for future)
    src/vis/isosurface.cpp
    src/vis/streamline.cpp
    src/vis/volume.cpp
    src/vis/slice.cpp
    src/vis/particle.cpp
)

target_include_directories(exd-render PUBLIC include)
target_include_directories(exd-render PRIVATE src)

target_link_libraries(exd-render PUBLIC
    exd::core
    OpenGL::GL
    SDL3::SDL3
)

add_library(exd::render ALIAS exd-render)

include(GNUInstallDirs)
install(TARGETS exd-render EXPORT exd-render-targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT exd-render-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/exd-render)
