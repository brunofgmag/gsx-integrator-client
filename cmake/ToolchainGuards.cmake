if (GSXI_TESTS_ONLY)
    return()
endif ()

if (NOT WIN32)
    message(FATAL_ERROR "gsx-integrator-client targets Windows (SimConnect requires it).")
endif ()

if (MSVC)
    if (MSVC_VERSION LESS 1930)
        message(FATAL_ERROR "Visual Studio 2022 (MSVC 19.30 or newer) is required.")
    endif ()
elseif (MINGW)
    message(STATUS "Configuring MinGW toolchain (Linux->Windows cross-compile / PoC path).")
else ()
    message(FATAL_ERROR
            "This project requires MSVC 2022 x64 (native) or MinGW (cross-compile); "
            "the selected compiler is neither.")
endif ()
