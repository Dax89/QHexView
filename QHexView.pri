QT += widgets

DEFINES += "QHEXVIEW_ENABLE_DIALOGS=1"

HEADERS += $$PWD/model/commands/hexcommand.h \
           $$PWD/model/commands/insertcommand.h \
           $$PWD/model/commands/removecommand.h \
           $$PWD/model/commands/replacecommand.h \
           $$PWD/model/buffer/qdevicebuffer.h \
           $$PWD/model/buffer/qhexbuffer.h \
           $$PWD/model/buffer/qmemorybuffer.h \
           $$PWD/model/buffer/qmemoryrefbuffer.h \
           $$PWD/model/buffer/qmappedfilebuffer.h \
           $$PWD/model/qhexdelegate.h \
           $$PWD/model/qhexutils.h \
           $$PWD/model/qhexcursor.h \
           $$PWD/model/qhexmetadata.h \
           $$PWD/model/qhexoptions.h \
           $$PWD/model/qhexdocument.h \
           $$PWD/dialogs/hexfinddialog.h \
           $$PWD/qhexview.h

SOURCES += $$PWD/model/commands/hexcommand.cpp \
           $$PWD/model/commands/insertcommand.cpp \
           $$PWD/model/commands/removecommand.cpp \
           $$PWD/model/commands/replacecommand.cpp \
           $$PWD/model/buffer/qdevicebuffer.cpp \
           $$PWD/model/buffer/qhexbuffer.cpp \
           $$PWD/model/buffer/qmemorybuffer.cpp \
           $$PWD/model/buffer/qmemoryrefbuffer.cpp \
           $$PWD/model/buffer/qmappedfilebuffer.cpp \
           $$PWD/model/qhexdelegate.cpp \
           $$PWD/model/qhexutils.cpp \
           $$PWD/model/qhexcursor.cpp \
           $$PWD/model/qhexmetadata.cpp \
           $$PWD/model/qhexdocument.cpp \
           $$PWD/dialogs/hexfinddialog.cpp \
           $$PWD/qhexview.cpp

INCLUDEPATH += $$PWD
