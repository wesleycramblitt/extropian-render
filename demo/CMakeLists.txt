# ── Assets ──────────────────────────────────────
include(FetchContent)
if(NOT DEFINED EXD_ASSETS_DIR)
    FetchContent_Declare(exd-assets
        GIT_REPOSITORY https://github.com/wesleycramblitt/extropian-assets.git
        GIT_TAG main
    )
    FetchContent_GetProperties(exd-assets)
    if(NOT exd-assets_POPULATED)
        FetchContent_Populate(exd-assets)
    endif()
    set(EXD_ASSETS_DIR "${exd-assets_SOURCE_DIR}")
endif()

# ── Geometry (for shape generation in the demo) ──
if(NOT TARGET exd::geometry)
    FetchContent_Declare(exd-geometry
        GIT_REPOSITORY https://github.com/wesleycramblitt/extropian-geometry.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(exd-geometry)
endif()

# ── Demo executable ─────────────────────────────
add_executable(extropian-render-demo main.cpp)
target_link_libraries(extropian-render-demo PRIVATE exd::render exd::geometry)
target_compile_definitions(extropian-render-demo PRIVATE
    EXD_ASSETS_DIR="${EXD_ASSETS_DIR}")
target_compile_features(extropian-render-demo PRIVATE cxx_std_23)
