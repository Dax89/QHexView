set(TARGET_NAME QHexViewExample)

add_executable(${TARGET_NAME}
    main.cpp
)

target_link_libraries(${TARGET_NAME}
    PRIVATE QHexView
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)
