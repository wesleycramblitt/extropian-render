
# Graphics infrastructure and technique implementations.

file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/techniques/*.cpp"
)

target_sources(exd-render PRIVATE ${MODULE_SOURCES})
