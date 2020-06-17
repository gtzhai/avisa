
#------------------------------------------------------------------------------
# gcc coverage

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    find_program(GCOV_EXE gcov)
    find_program(LCOV_EXE lcov)
    find_program(GENHTML_EXE genhtml)

    message(STATUS "Coverage:")

    option(COVERAGE "description" OFF)
    message(STATUS "  + COVERAGE                     ${COVERAGE}")
    if(COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

        # gcov
        if (NOT ${GCOV_EXE} STREQUAL GCOV_EXE-NOTFOUND)
          file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/coverage/gcov)
          message("-- gcov found, coverage reports available through target 'gcov'")
          add_custom_target(gcov
            COMMAND find ${PROJECT_BINARY_DIR} -type f -name *.gcno -exec ${GCOV_EXE} -pb {} '\;' > ${PROJECT_BINARY_DIR}/coverage/gcov/coverage.info
            COMMAND echo "Generated coverage report: " ${PROJECT_BINARY_DIR}/coverage/gcov/coverage.info
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/coverage/gcov
            )
        endif()

        # lcov
        if (NOT ${LCOV_EXE} STREQUAL LCOV_EXE-NOTFOUND AND NOT ${GENHTML_EXE} STREQUAL GENHTML_EXE-NOTFOUND)
          message("-- lcov and genhtml found, html reports available through target 'lcov'")
          file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/coverage/lcov/html)
          add_custom_target(lcov
            COMMAND lcov --capture --base-directory ${PROJECT_BINARY_DIR} --directory ${PROJECT_BINARY_DIR} --output-file ${PROJECT_BINARY_DIR}/coverage/lcov/coverage.info 
            COMMAND lcov --remove ${PROJECT_BINARY_DIR}/coverage/lcov/coverage.info 3rd/* src/unittest/* 5/* /usr/include/* /usr/lib/* --base-directory ${PROJECT_BINARY_DIR} --output-file ${PROJECT_BINARY_DIR}/coverage/lcov/coverage.info
            COMMAND genhtml ${PROJECT_BINARY_DIR}/coverage/lcov/coverage.info --output-directory ${PROJECT_BINARY_DIR}/coverage/lcov/html --demangle-cpp #--rc genhtml_branch_coverage=1
            COMMAND echo "HTML report generated in: " ${PROJECT_BINARY_DIR}/coverage/lcov/html
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            )
        endif()
    endif()

endif()
