#pragma once

#include <QHash>
#include <QColor>
#include <QChar>

#define QHEXVIEW_ADDRESSLABEL    ""
#define QHEXVIEW_ASCIILABEL      "0123456789ABCDEF"
#define QHEXVIEW_UNPRINTABLECHAR '.'
#define QHEXVIEW_LINELENGTH      0x10

struct QHexColor
{
    QColor foreground;
    QColor background;
};

struct QHexOptions
{
    // Appearance
    QChar unprintablechar{QHEXVIEW_UNPRINTABLECHAR};
    QString addresslabel{QHEXVIEW_ADDRESSLABEL};
    QString hexlabel;
    QString asciilabel{QHEXVIEW_ASCIILABEL};
    quint64 baseaddress{0};
    unsigned int linelength{QHEXVIEW_LINELENGTH};
    unsigned int grouplength{1};
    unsigned int scrollsteps{1};
    bool separators{false};
    bool header{true};

    // Colors & Styles
    QHash<quint8, QHexColor> bytecolors;
    QColor linealternatebackground;
    QColor linebackground;
    QColor headercolor;
    QColor commentcolor;
    QColor separatorcolor;
};
