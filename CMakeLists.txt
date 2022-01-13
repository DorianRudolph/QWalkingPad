cmake_minimum_required(VERSION 3.17)
project(qwalkinkpad2)

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

find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)
target_link_libraries(${PROJECT_NAME} ${REQUIRED_LIBS_QUALIFIED})
