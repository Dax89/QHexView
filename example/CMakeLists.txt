project(QHexView_Example)

add_executable(${PROJECT_NAME} main.cpp)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
        AUTOMOC ON
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        QHexView
)
