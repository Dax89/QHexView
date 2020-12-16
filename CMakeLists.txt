project(QHexView)

add_library(qhexview-lib STATIC
    document/buffer/qfilebuffer.cpp
    document/buffer/qfilebuffer.h
    document/buffer/qhexbuffer.cpp
    document/buffer/qhexbuffer.h
    document/buffer/qmemorybuffer.cpp
    document/buffer/qmemorybuffer.h
    document/buffer/qmemoryrefbuffer.cpp
    document/buffer/qmemoryrefbuffer.h
    document/commands/hexcommand.cpp
    document/commands/hexcommand.h
    document/commands/insertcommand.cpp
    document/commands/insertcommand.h
    document/commands/removecommand.cpp
    document/commands/removecommand.h
    document/commands/replacecommand.cpp
    document/commands/replacecommand.h
    document/qhexcursor.cpp
    document/qhexcursor.h
    document/qhexdocument.cpp
    document/qhexdocument.h
    document/qhexmetadata.cpp
    document/qhexmetadata.h
    document/qhexrenderer.cpp
    document/qhexrenderer.h
    qhexview.cpp
    qhexview.h
)

target_link_libraries(qhexview-lib PRIVATE Qt5::Widgets)
target_include_directories(qhexview-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
