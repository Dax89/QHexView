cmake_minimum_required(VERSION 3.13)

project(QHexView)

option(QHEXVIEW_ENABLE_DIALOGS "BuiltIn dialogs" OFF)
option(QHEXVIEW_ENABLE_EXAMPLE "Build the QHexView example program." OFF)

add_library(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Find Qt if not done so before.
if (NOT DEFINED QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)
endif ()

qt_wrap_cpp(MOC_SOURCES
    "model/buffer/qdevicebuffer.h"
    "model/buffer/qhexbuffer.h"
    "model/buffer/qmemorybuffer.h"
    "model/buffer/qmemoryrefbuffer.h"
    "model/qhexdelegate.h"
    "model/qhexcursor.h"
    "model/qhexmetadata.h"
    "model/qhexdocument.h"
    "qhexview.h"
)

target_sources(${PROJECT_NAME}
    PRIVATE
        "model/commands/hexcommand.h"
        "model/commands/hexcommand.cpp"
        "model/commands/insertcommand.h"
        "model/commands/insertcommand.cpp"
        "model/commands/removecommand.h"
        "model/commands/removecommand.cpp"
        "model/commands/replacecommand.h"
        "model/commands/replacecommand.cpp"
        "model/buffer/qdevicebuffer.h"
        "model/buffer/qdevicebuffer.cpp"
        "model/buffer/qhexbuffer.h"
        "model/buffer/qhexbuffer.cpp"
        "model/buffer/qmemorybuffer.h"
        "model/buffer/qmemorybuffer.cpp"
        "model/buffer/qmemoryrefbuffer.h"
        "model/buffer/qmemoryrefbuffer.cpp"
        "model/buffer/qmappedfilebuffer.h"
        "model/buffer/qmappedfilebuffer.cpp"
        "model/qhexdelegate.h"
        "model/qhexdelegate.cpp"
        "model/qhexutils.h"
        "model/qhexutils.cpp"
        "model/qhexcursor.h"
        "model/qhexcursor.cpp"
        "model/qhexmetadata.h"
        "model/qhexmetadata.cpp"
        "model/qhexoptions.h"
        "model/qhexdocument.cpp"
        "model/qhexdocument.h"
        "qhexview.cpp"
        ${MOC_SOURCES}
    PUBLIC
        "qhexview.h"
)

unset(MOC_SOURCES)


if(QHEXVIEW_ENABLE_DIALOGS)
    qt_wrap_cpp(MOC_SOURCES
        "dialogs/hexfinddialog.h"
    )

    target_sources(${PROJECT_NAME}
        PRIVATE
            "dialogs/hexfinddialog.h"
            "dialogs/hexfinddialog.cpp"
            ${MOC_SOURCES}
    )

    unset(MOC_SOURCES)

    target_compile_definitions(${PROJECT_NAME}
        PUBLIC QHEXVIEW_ENABLE_DIALOGS
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
)


if (QHEXVIEW_ENABLE_EXAMPLE)
    add_subdirectory(example)
endif ()
