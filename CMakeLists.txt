cmake_minimum_required(VERSION 3.16)

project(TitanPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Multimedia OpenGL Svg)

qt_standard_project_setup(REQUIRES 6.8)

set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_part/ffmpeg-8.1-full_build-shared")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT}/include")
set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/lib")
set(FFMPEG_BIN_DIR "${FFMPEG_ROOT}/bin")

set(FFMPEG_LIBS
    avcodec
    avformat
    avutil
    swscale
    swresample
)

qt_add_resources(APP_RESOURCES resources.qrc)

qt_add_executable(appTitanPlayer
    main.cpp
    core/audio_output.h
    core/audio_output.cpp
    core/ffmpeg_player.h
    core/ffmpeg_player.cpp
    core/player_controller.h
    core/player_controller.cpp
    core/video_frame_queue.h
    render/video_frame_source.h
    render/video_frame_source.cpp
    render/video_output_item.h
    render/video_output_item.cpp
    ${APP_RESOURCES}
)

qt_add_qml_module(appTitanPlayer
    URI TitanPlayer
    QML_FILES
        Main.qml
        qml/ControlBar.qml
        qml/IconButton.qml
        qml/PlaylistPanel.qml
        qml/SettingsDialog.qml
        qml/SpeedPopup.qml
        qml/TitleBar.qml
        qml/VideoArea.qml
        qml/VolumePopup.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appTitanPlayer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appTitanPlayer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTitanPlayer
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Multimedia
        Qt6::OpenGL
        Qt6::Svg
)

target_include_directories(appTitanPlayer PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIR}
)

foreach(lib ${FFMPEG_LIBS})
    target_link_libraries(appTitanPlayer PRIVATE "${FFMPEG_LIB_DIR}/${lib}.lib")
endforeach()

if(WIN32)
    add_custom_command(TARGET appTitanPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${FFMPEG_BIN_DIR}
            $<TARGET_FILE_DIR:appTitanPlayer>
        COMMENT "Copying FFmpeg DLLs to output directory"
    )
endif()

include(GNUInstallDirs)
install(TARGETS appTitanPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
