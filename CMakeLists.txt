cmake_minimum_required(VERSION 3.21)
project(extropian-render VERSION 0.1.0 LANGUAGES C CXX
    DESCRIPTION "Extropian Render: 3D rendering with OpenGL backend"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies: use FetchContent when not already available ──
include(FetchContent)

if(NOT TARGET exd::core)
    FetchContent_Declare(exd-core
        GIT_REPOSITORY https://github.com/wesleycramblitt/extropian-core.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(exd-core)
endif()

if(NOT TARGET exd::app)
    FetchContent_Declare(exd-app
        GIT_REPOSITORY https://github.com/wesleycramblitt/extropian-app.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(exd-app)
endif()
find_package(OpenGL REQUIRED)

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.18)
    FetchContent_MakeAvailable(SDL3)
endif()

add_library(exd-render STATIC
    # OpenGL backend — GPU resource management
    src/backend/opengl/gl_mesh.cpp
    src/backend/opengl/gl_shader.cpp
    src/backend/opengl/gl_texture.cpp
    src/backend/opengl/gl_renderer.cpp
    src/backend/opengl/gl_particle_volume.cpp

    # OpenGL backend — systems (renderer + mesh generators + scene systems)
    src/backend/opengl/gl_systems.cpp

    # Null backend
    src/backend/null/null_renderer.cpp

    # Core renderer infrastructure
    src/renderer.cpp
    src/camera.cpp
    src/render_graph.cpp

    # Sci-vis stubs (for future)
    src/vis/isosurface.cpp
    src/vis/streamline.cpp
    src/vis/volume.cpp
    src/vis/slice.cpp
    src/vis/particle.cpp
)

target_include_directories(exd-render PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_include_directories(exd-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# GLAD — vendored pre-generated OpenGL 4.6 Core loader (external/glad/)
target_include_directories(exd-render PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/include>")
target_sources(exd-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/src/gl.c)

target_link_libraries(exd-render PUBLIC
    exd::core
    exd::app
    OpenGL::GL
    SDL3::SDL3
)

add_library(exd::render ALIAS exd-render)

include(GNUInstallDirs)
install(TARGETS exd-render EXPORT exd-render-targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT exd-render-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/exd-render)
