
set(test_app "test_app")

add_executable(${test_app})
target_sources(
    ${test_app}
    PRIVATE
        "test_config.cpp"
)
target_link_libraries(${test_app} PRIVATE ${MAYONES_APP} GTest::gtest_main)
mayones_setup_target(${test_app} DISABLE_LTO DISABLE_WERROR)

gtest_discover_tests(${test_app})
