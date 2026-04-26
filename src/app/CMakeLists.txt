
configure_file("version.hpp.in" "${PROJECT_BINARY_DIR}/mayones/app/version.hpp")

add_library(${MAYONES_APP} STATIC)
target_sources(
    ${MAYONES_APP}
    PRIVATE
        "app.cpp"
        "config.cpp"
)
target_include_directories(${MAYONES_APP}
    PUBLIC
        "include"
    PRIVATE
        ${PROJECT_BINARY_DIR}
)
target_link_libraries(${MAYONES_APP} PUBLIC ${MAYONES_FLAGS})
mayones_setup_target(${MAYONES_APP})
