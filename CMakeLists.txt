cmake_minimum_required(VERSION 3.16)

project(SDC-Windows VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Network
    Concurrent
    Multimedia
)

add_subdirectory(3rdparty/opus)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/integration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/voice_engine
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opus/include
)

set(SOURCES
    src/main.cpp
    src/integration/PortalApiService.cpp
    src/integration/OAuthService.cpp
    src/integration/EncryptedStorage.cpp
    src/voice_engine/MumbleClientEngine.cpp
    src/voice_engine/MumbleProtobuf.cpp
    src/voice_engine/AudioEngine.cpp
    src/viewmodels/AppViewModel.cpp
    src/viewmodels/LoginViewModel.cpp
    src/viewmodels/ProfileViewModel.cpp
    src/viewmodels/VoiceViewModel.cpp
)

set(HEADERS
    src/integration/PortalApiService.h
    src/integration/OAuthService.h
    src/integration/EncryptedStorage.h
    src/voice_engine/MumbleClientEngine.h
    src/voice_engine/MumbleProtobuf.h
    src/voice_engine/AudioEngine.h
    src/viewmodels/AppViewModel.h
    src/viewmodels/LoginViewModel.h
    src/viewmodels/ProfileViewModel.h
    src/viewmodels/VoiceViewModel.h
)

set(RESOURCES
    qml.qrc
)

qt_add_executable(SDC-Windows
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(SDC-Windows PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
    Qt6::Multimedia
    opus
)

if(WIN32)
    target_link_libraries(SDC-Windows PRIVATE Crypt32)
endif()

# Hides black CMD console window on Windows GUI application
set_target_properties(SDC-Windows PROPERTIES
    WIN32_EXECUTABLE TRUE
)
