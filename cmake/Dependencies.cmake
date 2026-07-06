if (NOT CMAKE_PREFIX_PATH AND EXISTS "C:/Qt")
    file(GLOB QT_MSVC_KITS LIST_DIRECTORIES TRUE "C:/Qt/6.*/msvc2022_64")
    list(SORT QT_MSVC_KITS COMPARE NATURAL ORDER DESCENDING)
    if (QT_MSVC_KITS)
        list(GET QT_MSVC_KITS 0 AUTO_DETECTED_QT_ROOT)
        list(PREPEND CMAKE_PREFIX_PATH "${AUTO_DETECTED_QT_ROOT}")
        message(STATUS "Auto-detected Qt MSVC kit: ${AUTO_DETECTED_QT_ROOT}")
    endif ()
endif ()

if (GSXI_TESTS_ONLY)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Test WebSockets)
else ()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Quick WebSockets Network LinguistTools)
endif ()

qt_standard_project_setup(REQUIRES 6.8)

set(CMAKE_AUTORCC ON)

include(CTest)

if (BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
endif ()

if (NOT GSXI_TESTS_ONLY)
    if (DEFINED ENV{MSFS2024_SDK})
        file(TO_CMAKE_PATH "$ENV{MSFS2024_SDK}" MSFS_SDK)
    elseif (DEFINED ENV{MSFS_SDK})
        file(TO_CMAKE_PATH "$ENV{MSFS_SDK}" MSFS_SDK)
    else ()
        message(FATAL_ERROR "Missing MSFS or MSFS 2024 SDK (needed for SimConnect).")
    endif ()

    set(SIMCONNECT_INCLUDE_DIR "${MSFS_SDK}/SimConnect SDK/include"
            CACHE PATH "SimConnect include directory")
    set(SIMCONNECT_IMPORT_LIB "${MSFS_SDK}/SimConnect SDK/lib/SimConnect.lib"
            CACHE FILEPATH "SimConnect import library")
    set(SIMCONNECT_DLL "${MSFS_SDK}/SimConnect SDK/lib/SimConnect.dll"
            CACHE FILEPATH "SimConnect runtime DLL")

    if (NOT EXISTS "${SIMCONNECT_IMPORT_LIB}")
        message(FATAL_ERROR "SimConnect.lib not found at: ${SIMCONNECT_IMPORT_LIB}")
    endif ()
endif ()
