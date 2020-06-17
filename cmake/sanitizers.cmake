
#------------------------------------------------------------------------------
# Clang and gcc sanitizers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "Sanitizers:")

    option(ADDRESS_SANITIZER "description" OFF)
    message(STATUS "  + ADDRESS_SANITIZER                     ${ADDRESS_SANITIZER}")
    if(ADDRESS_SANITIZER)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        link_libraries(-fsanitize=address -fno-omit-frame-pointer)
    endif()

    option(UD_SANITIZER "description" OFF)
    message(STATUS "  + UB_SANITIZER                          ${UD_SANITIZER}")
    if(UD_SANITIZER)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall")
        add_compile_options(-fsanitize=undefined)
        link_libraries(-fsanitize=undefined)
    endif()

    option(THREAD_SANITIZER "description" OFF)
    message(STATUS "  + THREAD_SANITIZER                      ${THREAD_SANITIZER}")
    if(THREAD_SANITIZER)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall")
        add_compile_options(-fsanitize=thread)
        link_libraries(-fsanitize=thread)
    endif()

    # Clang only
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        configure_file(${PROJECT_SOURCE_DIR}/cmake/SanitizerBlacklist.txt.in ${PROJECT_BINARY_DIR}/SanitizerBlacklist.txt)
        add_compile_options(-fsanitize-blacklist=${PROJECT_BINARY_DIR}/SanitizerBlacklist.txt)

        option(MEMORY_SANITIZER "description" OFF)
        message(STATUS "  + MEMORY_SANITIZER                      ${MEMORY_SANITIZER}")
        if(MEMORY_SANITIZER)
            add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
            link_libraries(-fsanitize=memory -fno-omit-frame-pointer)
        endif()
    endif()
endif()

