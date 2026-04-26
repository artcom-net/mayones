
add_executable(${MAYONES_EXE})
target_sources(
    ${MAYONES_EXE}
    PRIVATE
        "main.cpp"
)
target_link_libraries(${MAYONES_EXE} PRIVATE ${MAYONES_APP})
mayones_setup_target(${MAYONES_EXE})
