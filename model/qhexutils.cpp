#include "qhexutils.h"
#include "qhexoptions.h"
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

}
