# ── doctest (single-header download) ─────────────
set(DOCTEST_DIR "${CMAKE_BINARY_DIR}/_deps/doctest-src")
if(NOT EXISTS "${DOCTEST_DIR}/doctest/doctest.h")
    file(DOWNLOAD
        https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
        "${DOCTEST_DIR}/doctest/doctest.h"
        SHOW_PROGRESS
    )
endif()

# ── Copy shaders to build dir for tests ──────────
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../shaders)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../shaders DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ── Non-GL unit tests ────────────────────────────
add_executable(test-render-unit
    test_main.cpp
    test_camera.cpp
    test_mesh.cpp
    test_components.cpp
    test_render_graph.cpp
    test_ecs_systems.cpp
    test_ray.cpp
    test_selection.cpp
)
target_include_directories(test-render-unit PRIVATE ${DOCTEST_DIR})
target_link_libraries(test-render-unit PRIVATE exd::render)
target_compile_features(test-render-unit PRIVATE cxx_std_23)

# ── GL rendering tests ───────────────────────────
add_executable(test-render-gl
    test_main.cpp
    test_techniques.cpp
    test_window.cpp
    test_interaction.cpp
)
target_include_directories(test-render-gl PRIVATE ${DOCTEST_DIR})
target_link_libraries(test-render-gl PRIVATE exd::render)
target_compile_features(test-render-gl PRIVATE cxx_std_23)

# ── CTest registration ───────────────────────────
add_test(NAME render_unit COMMAND test-render-unit
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME render_gl   COMMAND test-render-gl
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
