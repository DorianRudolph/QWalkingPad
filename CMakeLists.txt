cmake_minimum_required(VERSION 3.17)
project(qwalkinkpad)

set(CMAKE_PREFIX_PATH "C:\\Qt\\6.2.2\\msvc2019_64")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_VERSION 6)
set(REQUIRED_LIBS Core Gui Widgets Bluetooth)
set(REQUIRED_LIBS_QUALIFIED Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Bluetooth)

add_executable(${PROJECT_NAME} main.cpp MainWindow.cpp Protocol.cpp Statistics.cpp)

set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION}) # version for SingleApplication

add_subdirectory(singleapplication)
target_link_libraries(${PROJECT_NAME} SingleApplication::SingleApplication)

add_subdirectory(QHotkey)
target_link_libraries(${PROJECT_NAME} qhotkey)

find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)
target_link_libraries(${PROJECT_NAME} ${REQUIRED_LIBS_QUALIFIED})

if (MSVC)
    set(DEBUG_SUFFIX)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        file(COPY "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll" DESTINATION "${CMAKE_BINARY_DIR}/plugins/platforms/")
    endif ()
    foreach (QT_LIB ${REQUIRED_LIBS} Network)
        file(COPY "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll" DESTINATION "${CMAKE_BINARY_DIR}")
    endforeach (QT_LIB)
endif()