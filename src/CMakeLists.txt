add_library(myshell_core STATIC
    lexer/token.cc
    lexer/lexer.cc
    parser/parser.cc
    history/history.cc
    terminal/prompt.cc
    terminal/line_editor.cc
    completion/completion.cc
    builtins/builtin.cc
    execution/executor.cc
    support/logger.cc
)

target_include_directories(myshell_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(myshell_core PUBLIC
    Threads::Threads
)

if (LIBEDIT_FOUND)
    target_include_directories(myshell_core PRIVATE ${LIBEDIT_INCLUDE_DIRS})
    target_link_libraries(myshell_core PRIVATE ${LIBEDIT_LIBRARIES})
endif()

if (MYSHELL_ENABLE_WARNINGS)
    set_project_warnings(myshell_core)
endif()
enable_sanitizers(myshell_core)

add_executable(myshell
    app/main.cc
)

target_link_libraries(myshell PRIVATE
    myshell_core
)

if (MYSHELL_ENABLE_WARNINGS)
    set_project_warnings(myshell)
endif()
enable_sanitizers(myshell)
