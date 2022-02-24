#pragma once

#include <QHash>
#include <QColor>
#include <QChar>

#define QHEXVIEW_ADDRESSLABEL    "OFFSET"
#define QHEXVIEW_ASCIILABEL      "ASCII"
#define QHEXVIEW_UNPRINTABLECHAR '.'
#define QHEXVIEW_LINELENGTH      0x10

struct QHexOptions
{
    // Appearance
    QChar unprintablechar{QHEXVIEW_UNPRINTABLECHAR};
    QString addresslabel{QHEXVIEW_ADDRESSLABEL};
    QString asciilabel{QHEXVIEW_ASCIILABEL};
    unsigned int linelength{QHEXVIEW_LINELENGTH};
    unsigned int grouplength{1};
    int scrollsteps{1};
    bool header{true};

    // Colors
    QHash<quint8, QColor> bytecolors;
    QColor headercolor;
};
