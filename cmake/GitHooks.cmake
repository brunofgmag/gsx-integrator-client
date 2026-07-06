if (DEFINED ENV{CI})
    return()
endif ()

find_package(Git QUIET)
if (NOT Git_FOUND)
    return()
endif ()

execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --is-inside-work-tree
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GSXI_IN_WORK_TREE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT GSXI_IN_WORK_TREE STREQUAL "true")
    return()
endif ()

execute_process(
        COMMAND "${GIT_EXECUTABLE}" config --get core.hooksPath
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GSXI_HOOKS_PATH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GSXI_HOOKS_PATH STREQUAL ".githooks")
    return()
endif ()

execute_process(
        COMMAND "${GIT_EXECUTABLE}" config core.hooksPath .githooks
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE GSXI_HOOKS_RESULT)
if (GSXI_HOOKS_RESULT EQUAL 0)
    message(STATUS "Git hooks configured: core.hooksPath = .githooks")
else ()
    message(WARNING "Could not configure git hooks (core.hooksPath).")
endif ()
