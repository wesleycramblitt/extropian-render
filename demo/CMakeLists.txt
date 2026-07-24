add_executable(extropian-render-demo main.cpp)
target_link_libraries(extropian-render-demo PRIVATE exd::render)
target_compile_definitions(extropian-render-demo PRIVATE
    EXD_ASSETS_DIR="${EXD_ASSETS_DIR}")
target_compile_features(extropian-render-demo PRIVATE cxx_std_23)
