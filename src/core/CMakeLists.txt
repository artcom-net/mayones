
add_library(${MAYONES_CORE} STATIC)
target_sources(
    ${MAYONES_CORE}
    PRIVATE
        "cartridge.cpp"
        "core.cpp"
        "mapper.cpp"
        "rom_loader.cpp"
)
target_include_directories(${MAYONES_CORE} PUBLIC "include")
target_link_libraries(${MAYONES_CORE} PUBLIC ${MAYONES_FLAGS})
mayones_setup_target(${MAYONES_CORE})
