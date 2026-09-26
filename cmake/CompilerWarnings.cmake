function(set_project_warnings target_name)
    set(GCC_CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target_name} PRIVATE ${GCC_CLANG_WARNINGS})
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            ${GCC_CLANG_WARNINGS}
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
endfunction()
