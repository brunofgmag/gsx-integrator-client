qt_add_executable(${APP_NAME} ${APP_SOURCES})

set_source_files_properties(src/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${APP_NAME}
        URI GsxIntegratorClient
        VERSION 1.0
        QML_FILES
        ${APP_QML_FILES}
)

qt_add_translations(${APP_NAME}
        TS_FILES i18n/app_en.ts i18n/app_pt_BR.ts
        RESOURCE_PREFIX "/i18n"
)

target_include_directories(${APP_NAME} SYSTEM PRIVATE
        "${SIMCONNECT_INCLUDE_DIR}"
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/VERSION.txt")
file(READ "${CMAKE_SOURCE_DIR}/VERSION.txt" GSXI_VERSION)
string(STRIP "${GSXI_VERSION}" GSXI_VERSION)

target_compile_definitions(${APP_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        GSXI_VERSION="${GSXI_VERSION}"
        $<$<CONFIG:Release>:NDEBUG>
)

if (MSVC)
    target_compile_options(${APP_NAME} PRIVATE /W4 /permissive- /Zc:preprocessor /external:W0 /wd4702)
endif ()

target_precompile_headers(${APP_NAME} PRIVATE
        <QtCore/QtCore>
        <QtQml/QtQml>
        <QtQuick/QtQuick>
        <QtNetwork/QtNetwork>
        <QtWebSockets/QtWebSockets>
        <memory>
        <optional>
        <string>
        <vector>
)

target_link_libraries(${APP_NAME} PRIVATE
        Qt6::Quick
        Qt6::Network
        Qt6::WebSockets
        "${SIMCONNECT_IMPORT_LIB}"
        dwmapi
)

if (MINGW)
    target_compile_definitions(${APP_NAME} PRIVATE MINGW_HAS_SECURE_API)
endif ()

add_library(gsxi-header-self-containment OBJECT
        tools/header-self-containment/IntegratorSnapshot.cpp)
target_include_directories(gsxi-header-self-containment PRIVATE
        "${CMAKE_SOURCE_DIR}/src")
add_dependencies(${APP_NAME} gsxi-header-self-containment)
