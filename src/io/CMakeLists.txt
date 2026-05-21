
add_library(${MAYONES_IO} STATIC)
target_sources(
    ${MAYONES_IO}
    PRIVATE
        "file.cpp"
)
target_include_directories(${MAYONES_IO}
    PUBLIC
        "include"
)
target_link_libraries(${MAYONES_IO} PUBLIC ${MAYONES_FLAGS})
mayones_setup_target(${MAYONES_IO})
