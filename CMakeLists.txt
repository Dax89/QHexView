cmake_minimum_required(VERSION 3.0)

project(QHexView)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE SOURCES *.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_BUILD_TYPE MATCHES "Debug") # Enforce warnings as errors if not in Release mode
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX /permissive)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -Wno-error=extra -Wshadow -pedantic)
    endif()
endif()
