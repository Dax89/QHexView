#pragma once

#include <QHash>
#include <QColor>
#include <QChar>

#define QHEXVIEW_ADDRESSLABEL    ""
#define QHEXVIEW_ASCIILABEL      "0123456789ABCDEF"
#define QHEXVIEW_UNPRINTABLECHAR '.'
#define QHEXVIEW_LINELENGTH      0x10

namespace QHexFlags {
    enum: unsigned int {
        None          = (1 << 0),
        HSeparator    = (1 << 1),
        VSeparator    = (1 << 2),
        StyledHeader  = (1 << 3),
        StyledAddress = (1 << 4),
        NoHeader      = (1 << 5),

        Separators    = HSeparator | VSeparator,
        Styled        = StyledHeader | StyledAddress,
    };
}

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
    unsigned int flags{QHexFlags::None};
    unsigned int linelength{QHEXVIEW_LINELENGTH};
    unsigned int grouplength{1};
    unsigned int scrollsteps{1};

    // Colors & Styles
    QHash<quint8, QHexColor> bytecolors;
    QColor linealternatebackground;
    QColor linebackground;
    QColor headercolor;
    QColor commentcolor;
    QColor separatorcolor;

    inline bool hasFlag(unsigned int flag) const { return flags & flag; }
};
