cmake_minimum_required(VERSION 3.21)
project(extropian-render VERSION 0.1.0 LANGUAGES C CXX
    DESCRIPTION "Extropian Render: 3D rendering with OpenGL backend"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies: use FetchContent when not already available ──
include(FetchContent)

# Some dependencies (doctest) use cmake_minimum_required(VERSION 3.0)
# which is rejected by CMake >= 4.0 unless we set this policy.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")

# ── Platform detection ─────────────────────────────────────
if(EMSCRIPTEN)
    message(STATUS "Target: WebGL (Emscripten)")
    set(EXD_PLATFORM_WEBGL ON)
else()
    message(STATUS "Target: Desktop (${CMAKE_SYSTEM_NAME})")
endif()

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

if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
endif()

if(NOT EMSCRIPTEN)
    find_package(SDL3 CONFIG QUIET)
    if(NOT SDL3_FOUND)
        include(FetchContent)
        FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.18)
        FetchContent_MakeAvailable(SDL3)
    endif()
endif()

# nlohmann/json — header-only JSON parser for environment configs
if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif()

add_library(exd-render STATIC)

add_subdirectory(src)
add_subdirectory(src/systems)
add_subdirectory(src/backends)
add_subdirectory(src/graphics)
add_subdirectory(src/interaction)

target_include_directories(exd-render PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_include_directories(exd-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# stb_image for texture loading
target_include_directories(exd-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image/include)

# GLAD — vendored pre-generated OpenGL 4.6 Core loader (external/glad/)
target_include_directories(exd-render PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/include>")
if(NOT EMSCRIPTEN)
    target_sources(exd-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/src/gl.c)
else()
    message(STATUS "Emscripten: using native GLES3, skipping GLAD loader")
endif()

target_link_libraries(exd-render PUBLIC
    assimp::assimp
    nlohmann_json::nlohmann_json
    exd::core
)

if(NOT EMSCRIPTEN)
    target_link_libraries(exd-render PUBLIC
        OpenGL::GL
        SDL3::SDL3
    )
else()
    message(STATUS "Emscripten: GLES3 is native, no SDL3 (browser handles input)")
    target_compile_options(exd-render PUBLIC "-sUSE_WEBGL2=1" "-sFULL_ES3=1")
    target_link_options(  exd-render PUBLIC "-sUSE_WEBGL2=1" "-sFULL_ES3=1")
endif()

add_library(exd::render ALIAS exd-render)

# ── Tests ─────────────────────────────────────────
option(EXD_RENDER_BUILD_TESTS "Build render tests" ON)
if(EXD_RENDER_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

# ── Demo ──────────────────────────────────────────
option(EXD_RENDER_BUILD_DEMO "Build render demo application" ON)
if(EXD_RENDER_BUILD_DEMO)
    add_subdirectory(demo)
endif()

#include(GNUInstallDirs)
#install(TARGETS exd-render EXPORT exd-render-targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#install(EXPORT exd-render-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/exd-render)

# Assimp for mesh file loading
if(NOT TARGET assimp::assimp)
    include(FetchContent)
    FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
    )
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(assimp)
    # Suppress GCC 16 warnings-as-errors in pre-existing assimp v5.4.3 code
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "16")
        target_compile_options(assimp PRIVATE -Wno-error)
    endif()
endif()
