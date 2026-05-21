
set(test_core "test_core")

add_executable(${test_core})
target_sources(
    ${test_core}
    PRIVATE
        "test_load_rom.cpp"
        "test_cpu.cpp"
)
target_link_libraries(${test_core} PRIVATE ${MAYONES_CORE} ${MAYONES_IO} GTest::gtest_main)
mayones_setup_target(${test_core} DISABLE_LTO DISABLE_WERROR)

gtest_discover_tests(${test_core})
