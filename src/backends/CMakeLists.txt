
file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/null/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/webgl/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/opengl/*.cpp"
)

target_sources(exd-render PRIVATE ${MODULE_SOURCES})
