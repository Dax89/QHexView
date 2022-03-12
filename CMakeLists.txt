cmake_minimum_required(VERSION 3.0)

project(QHexView)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

option(QHEXVIEW_ENABLE_DIALOGS "BuiltIn dialogs" OFF)

file(GLOB_RECURSE BASE_HEADERS "model/*.h" "qhexview.h")
file(GLOB_RECURSE BASE_SOURCES "model/*.cpp" "qhexview.cpp")

if(QHEXVIEW_ENABLE_DIALOGS)
    file(GLOB DIALOG_HEADERS "dialogs/*.h")
    file(GLOB DIALOG_SOURCES "dialogs/*.cpp")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${BASE_HEADERS} ${BASE_SOURCES} ${DIALOG_HEADERS} ${DIALOG_SOURCES})
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(QHEXVIEW_ENABLE_DIALOGS)
    target_compile_definitions(${PROJECT_NAME} INTERFACE QHEXVIEW_ENABLE_DIALOGS)
endif()
