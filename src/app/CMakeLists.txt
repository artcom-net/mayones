
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

configure_file("version.hpp.in" "${PROJECT_BINARY_DIR}/mayones/app/version.hpp")

add_library(${MAYONES_APP} STATIC)
target_sources(
    ${MAYONES_APP}
    PRIVATE
        "app.cpp"
        "config.cpp"
        "render.cpp"
)
target_include_directories(${MAYONES_APP}
    PUBLIC
        "include"
    PRIVATE
        ${PROJECT_BINARY_DIR}
)
target_link_libraries(${MAYONES_APP}
    PUBLIC
        ${MAYONES_FLAGS}
        ${MAYONES_CORE}
        ${MAYONES_IO}
    PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
)
mayones_setup_target(${MAYONES_APP})
