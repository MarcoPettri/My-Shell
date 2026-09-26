add_executable(unit_tests
    unit/test_lexer.cc
    unit/test_parser.cc
    unit/test_history.cc
    unit/test_concurrency.cc
    unit/test_execution.cc
)

target_link_libraries(unit_tests PRIVATE
    myshell_core
    GTest::gtest_main
)

if (MYSHELL_ENABLE_WARNINGS)
    set_project_warnings(unit_tests)
endif()
enable_sanitizers(unit_tests)

include(GoogleTest)
gtest_discover_tests(unit_tests)
