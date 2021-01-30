QT += widgets

HEADERS += $$PWD/document/commands/hexcommand.h \
           $$PWD/document/commands/insertcommand.h \
           $$PWD/document/commands/removecommand.h \
           $$PWD/document/commands/replacecommand.h \
           $$PWD/document/buffer/qhexbuffer.h \
           $$PWD/document/buffer/qmemoryrefbuffer.h \
           $$PWD/document/buffer/qmemorybuffer.h \
           $$PWD/document/buffer/qfilebuffer.h \
           $$PWD/document/qhexcursor.h \
           $$PWD/document/qhexdocument.h \
           $$PWD/document/qhexmetadata.h \
           $$PWD/document/qhexrenderer.h \
           $$PWD/qhexview.h

SOURCES += $$PWD/document/commands/hexcommand.cpp \
           $$PWD/document/commands/insertcommand.cpp \
           $$PWD/document/commands/removecommand.cpp \
           $$PWD/document/commands/replacecommand.cpp \
           $$PWD/document/buffer/qhexbuffer.cpp \
           $$PWD/document/buffer/qmemoryrefbuffer.cpp \
           $$PWD/document/buffer/qmemorybuffer.cpp \
           $$PWD/document/buffer/qfilebuffer.cpp \
           $$PWD/document/qhexcursor.cpp \
           $$PWD/document/qhexdocument.cpp \
           $$PWD/document/qhexmetadata.cpp \
           $$PWD/document/qhexrenderer.cpp \
           $$PWD/qhexview.cpp

INCLUDEPATH += $$PWD
