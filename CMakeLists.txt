cmake_minimum_required(VERSION 3.5)

project(Music_App VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------
# Qt setup
# -------------------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

# -------------------------------
# FFmpeg Hardcoded Paths (Manual Override)
# -------------------------------
# We define the paths directly to avoid any detection issues.
set(FFMPEG_INCLUDE_DIR "C:/vcpkg/installed/x64-mingw-dynamic/include")
set(FFMPEG_LIBRARY_DIR "C:/vcpkg/installed/x64-mingw-dynamic/lib")
set(FFMPEG_LIBRARIES avformat avcodec avutil swresample swscale)

message(STATUS "Using hardcoded FFmpeg include path: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "Using hardcoded FFmpeg library path: ${FFMPEG_LIBRARY_DIR}")

# -------------------------------
# Sources
# -------------------------------
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/appconfig.cpp
    src/appconfig.h
    src/audioerror.h
    src/audioengine.cpp
    src/audioengine.h
    src/ffmpegaudioengine.cpp
    src/ffmpegaudioengine.h
    src/audioiodevice.cpp
    src/audioiodevice.h
    src/audioimportdialog.cpp
    src/audioimportdialog.h
    src/transportdock.cpp
    src/transportdock.h
    timelinewidget/timelinewidget.cpp
    timelinewidget/timelinewidget.h
    timelinewidget/audioitem.cpp
    timelinewidget/audioitem.h
    timelinewidget/track.cpp
    timelinewidget/track.h
    timelinewidget/TimelineIndicator.cpp
    timelinewidget/TimelineIndicator.h
    timelinewidget/trackheaderwidget.cpp
    timelinewidget/trackheaderwidget.h
    timelinewidget/tracksettingsdialog.cpp
    timelinewidget/tracksettingsdialog.h
)

# -------------------------------
# Target
# -------------------------------
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Music_App
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Music_App ${PROJECT_SOURCES})
endif()

# -------------------------------
# Linking
# -------------------------------
target_link_libraries(Music_App PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

# --- Direct linking for FFmpeg ---
# We add the paths and libraries directly to the target.
target_include_directories(Music_App PRIVATE ${FFMPEG_INCLUDE_DIR})
target_link_directories(Music_App PRIVATE ${FFMPEG_LIBRARY_DIR})
target_link_libraries(Music_App PRIVATE ${FFMPEG_LIBRARIES})
target_compile_definitions(Music_App PRIVATE HAVE_FFMPEG=1)


# -------------------------------
# Bundle / install
# -------------------------------
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Music_App)
endif()
set_target_properties(Music_App PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Music_App
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Music_App)
endif()
