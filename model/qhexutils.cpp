#include "qhexutils.h"
#include "qhexoptions.h"
#include "../qhexview.h"
#include <array>

namespace QHexUtils {

static const std::array<char, 16> HEXMAP = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

QByteArray toHex(const QByteArray& ba, char sep)
{
    QByteArray hex(sep ? (ba.size() * 3 - 1) : (ba.size() * 2), Qt::Uninitialized);

    for(auto i = 0, o = 0; i < ba.size(); i++)
    {
        if(sep && i) hex[o++] = static_cast<uchar>(sep);
        hex[o++] = HEXMAP.at((ba.at(i) & 0xf0) >> 4);
        hex[o++] = HEXMAP.at(ba.at(i) & 0x0f);
    }

    return hex;
}

QByteArray toHex(const QByteArray& ba) { return QHexUtils::toHex(ba, '\0'); }
qint64 positionToOffset(const QHexOptions* options, HexPosition pos) { return options->linelength * pos.line + pos.column; }
HexPosition offsetToPosition(const QHexOptions* options, qint64 offset) { return { offset / options->linelength, offset % options->linelength }; }

std::pair<qint64, qint64> find(const QHexView* hexview, QVariant value, HexFindMode mode, HexFindDirection fd)
{
    QByteArray v;

    switch(mode)
    {
        case HexFindMode::Text: {
            if(value.type() == QVariant::String) v = value.toString().toUtf8();
            else if(value.type() == QVariant::ByteArray) v = value.toByteArray();
            else return {-1, 0};
            break;
        }

        case HexFindMode::Hex: {
            if(value.type() == QVariant::String) v = value.toString().toUtf8();
            else if(value.type() == QVariant::ByteArray) v = value.toByteArray();
            else return {-1, 0};
            break;
        }

        default: return {-1, 0};
    }

    QHexDocument* hexdocument = hexview->hexDocument();
    QHexCursor* hexcursor = hexview->hexCursor();

    qint64 startpos = 0;
    if(fd != HexFindDirection::All) startpos = hexcursor->hasSelection() ? hexcursor->selectionStartOffset() : hexcursor->offset();

    qint64 offset = fd == HexFindDirection::Backward ? hexdocument->lastIndexOf(v, startpos) :
                                                       hexdocument->indexOf(v, startpos);

    return {offset, offset > -1 ? v.size() : 0};
}

}
