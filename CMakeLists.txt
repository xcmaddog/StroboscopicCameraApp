cmake_minimum_required(VERSION 3.16)
project(IDSStrobeCameraTest VERSION 0.1 LANGUAGES CXX)

# ── Qt code generation ────────────────────────────────────────────────────────
# These three are required by Qt Creator's generated projects and must stay.
# AUTOMOC runs moc on any class with Q_OBJECT.
# AUTOUIC runs uic on .ui files.
# AUTORCC runs rcc on .qrc files.
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt ───────────────────────────────────────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ── IDS peak SDK ─────────────────────────────────────────────────────────────
# Adjust IDS_PEAK_ROOT if your installation path differs.
# Default for a standard IDS peak install on Windows:
set(IDS_PEAK_ROOT "C:/Program Files/IDS/ids_peak" CACHE PATH "IDS peak SDK root")

set(IDS_PEAK_INCLUDE "${IDS_PEAK_ROOT}/generic_sdk/api/include")
set(IDS_PEAK_LIB_DIR "${IDS_PEAK_ROOT}/generic_sdk/api/lib/x86_64")
set(IDS_PEAK_IPL_LIB_DIR "${IDS_PEAK_ROOT}/generic_sdk/ipl/lib/x86_64")
set(IDS_PEAK_IPL_INCLUDE "${IDS_PEAK_ROOT}/generic_sdk/ipl/include")
set(IDS_PEAK_BIN_DIR "${IDS_PEAK_ROOT}/generic_sdk/api/bin/x86_64")
set(IDS_PEAK_IPL_BIN_DIR "${IDS_PEAK_ROOT}/generic_sdk/ipl/bin/x86_64")

find_library(IDS_PEAK_LIB
    NAMES ids_peak
    PATHS "${IDS_PEAK_LIB_DIR}"
    REQUIRED
)
find_library(IDS_PEAK_IPL_LIB
    NAMES ids_peak_ipl
    PATHS "${IDS_PEAK_IPL_LIB_DIR}"
    REQUIRED
)

# ── Sources ───────────────────────────────────────────────────────────────────
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    cameracontroller.cpp
    cameracontroller.h

    camerawidget.cpp
    camerawidget.h

    cameraworker.cpp
    cameraworker.h

    liveviewwidget.cpp
    liveviewwidget.h

    strobewidget.cpp
    strobewidget.h
)

# ── Executable ────────────────────────────────────────────────────────────────
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(IDSStrobeCameraTest
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(IDSStrobeCameraTest SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(IDSStrobeCameraTest
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ── Include paths & linking ───────────────────────────────────────────────────
target_include_directories(IDSStrobeCameraTest PRIVATE
    "${IDS_PEAK_INCLUDE}"
    "${IDS_PEAK_IPL_INCLUDE}"
    "${IDS_PEAK_ROOT}/common/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(IDSStrobeCameraTest PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    "${IDS_PEAK_LIB}"
    "${IDS_PEAK_IPL_LIB}"
)

# ── Windows: copy IDS peak DLLs next to the executable ───────────────────────
# Without this the exe won't launch unless the IDS peak bin dir is on PATH.
if(WIN32)
    set(PEAK_DLL "${IDS_PEAK_BIN_DIR}/ids_peak.dll")
    set(PEAK_IPL_DLL "${IDS_PEAK_IPL_BIN_DIR}/ids_peak_ipl.dll")

    if(EXISTS "${PEAK_DLL}")
        file(COPY "${PEAK_DLL}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()

    if(EXISTS "${PEAK_IPL_DLL}")
        file(COPY "${PEAK_IPL_DLL}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
endif()

# ── Bundle / install settings (from Qt Creator template) ──────────────────────
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.IDSStrobeCameraTest)
endif()

set_target_properties(IDSStrobeCameraTest PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS IDSStrobeCameraTest
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(IDSStrobeCameraTest)
endif()
