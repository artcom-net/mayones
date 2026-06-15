
add_library(${MAYONES_CORE} STATIC)
target_sources(
    ${MAYONES_CORE}
    PRIVATE
        "bus.cpp"
        "cpu.cpp"
        "core.cpp"
        "mapper.cpp"
        "ppu.cpp"
        "ppu_bus.cpp"
        "rom.cpp"
        "rom_loader.cpp"
)
target_include_directories(${MAYONES_CORE} PUBLIC "include")
target_link_libraries(${MAYONES_CORE} PUBLIC ${MAYONES_FLAGS})
mayones_setup_target(${MAYONES_CORE})
