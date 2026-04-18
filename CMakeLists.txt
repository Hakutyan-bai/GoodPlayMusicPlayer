cmake_minimum_required(VERSION 3.21)

project(MusicPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Multimedia
    Qml
    Quick
    QuickControls2
    Widgets
)

qt_standard_project_setup(REQUIRES 6.8)

set(MUSICPLAYER_SOURCES
    src/ImportScanWorker.cpp
    src/ImportScanWorker.h
    src/main.cpp
    src/MusicPlayerController.cpp
    src/MusicPlayerController.h
    src/SpectrumAnalyzer.cpp
    src/SpectrumAnalyzer.h
    src/TrackImportUtils.cpp
    src/TrackImportUtils.h
    src/TrackListModel.cpp
    src/TrackListModel.h
    resources/app_resources.qrc
)

if (WIN32)
    list(APPEND MUSICPLAYER_SOURCES resources/app_icon.rc)
endif()

qt_add_executable(musicplayer
    ${MUSICPLAYER_SOURCES}
)

qt_add_qml_module(musicplayer
    URI MusicPlayer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/ActionButton.qml
        qml/components/GlassPanel.qml
        qml/components/MediaSlider.qml
        qml/components/SpectrumRing.qml
        qml/components/TitleBarButton.qml
)

target_link_libraries(musicplayer
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Widgets
)

set_target_properties(musicplayer PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if (WIN32)
    get_filename_component(QT_ROOT_DIR "${Qt6_DIR}/../../.." ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_ROOT_DIR}/bin"
        REQUIRED
    )

    add_custom_command(TARGET musicplayer POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
            --no-translations
            "$<TARGET_FILE:musicplayer>"
        COMMENT "Deploying Qt runtime dependencies with windeployqt"
        VERBATIM
    )
endif()
