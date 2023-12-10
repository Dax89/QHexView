cmake_minimum_required(VERSION 3.13)

project(QHexView)

option(QHEXVIEW_ENABLE_DIALOGS "BuiltIn dialogs" OFF)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${PROJECT_NAME}
    INTERFACE 
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
        "qhexview.h"
        "qhexview.cpp"
)

if(QHEXVIEW_ENABLE_DIALOGS)
    target_sources(${PROJECT_NAME}
        INTERFACE
            "dialogs/hexfinddialog.h"
            "dialogs/hexfinddialog.cpp"
    )

    target_compile_definitions(${PROJECT_NAME} INTERFACE QHEXVIEW_ENABLE_DIALOGS)
endif()
