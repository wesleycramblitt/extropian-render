cmake_minimum_required(VERSION 3.21)
project(extropian-render VERSION 0.1.0 LANGUAGES CXX
    DESCRIPTION "Extropian Render: 3D rendering abstraction with OpenGL/Vulkan/WebGL backends"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ──────────────────────────────────────
find_package(ext-core REQUIRED)
find_package(OpenGL REQUIRED)

# SDL3 for window/context creation (OpenGL backend)
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.18)
    FetchContent_MakeAvailable(SDL3)
endif()

# ── Library target ────────────────────────────────────
add_library(ext-render STATIC
    # Core
    src/renderer.cpp
    src/render_graph.cpp
    src/camera.cpp
    src/material.cpp

    # Sci-vis
    src/vis/isosurface.cpp
    src/vis/streamline.cpp
    src/vis/volume.cpp
    src/vis/slice.cpp
    src/vis/particle.cpp

    # Backend: OpenGL
    src/backend/opengl/gl_renderer.cpp
    src/backend/opengl/gl_mesh.cpp
    src/backend/opengl/gl_texture.cpp
    src/backend/opengl/gl_shader.cpp

    # Backend: Null
    src/backend/null/null_renderer.cpp
)

target_include_directories(ext-render PUBLIC include)
target_include_directories(ext-render PRIVATE src)

target_link_libraries(ext-render PUBLIC
    ext::core
    OpenGL::GL
    SDL3::SDL3
)

add_library(ext::render ALIAS ext-render)

# ── Install ───────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS ext-render EXPORT ext-render-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY shaders/ DESTINATION ${CMAKE_INSTALL_DATADIR}/ext-render/shaders)
install(EXPORT ext-render-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ext-render)

# ── Tests ─────────────────────────────────────────────
option(EXT_RENDER_BUILD_TESTS "Build tests" OFF)
if(EXT_RENDER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
