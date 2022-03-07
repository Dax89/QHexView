#pragma once

#include <QByteArray>
#include <QString>

struct QHexOptions;

enum class HexFindDirection { Forward, Backward };

struct HexPosition {
    qint64 line; qint64 column;
    static inline HexPosition invalid() { return {-1, -1}; }
    inline bool isValid() const { return line >= 0 && column >= 0; }
    inline bool operator==(const HexPosition& rhs) const { return (line == rhs.line) && (column == rhs.column); }
    inline bool operator!=(const HexPosition& rhs) const { return (line != rhs.line) || (column != rhs.column); }
};

namespace QHexUtils {

QByteArray toHex(const QByteArray& ba, char sep);
QByteArray toHex(const QByteArray& ba);
qint64 positionToOffset(const QHexOptions* options, HexPosition pos);
HexPosition offsetToPosition(const QHexOptions* options, qint64 offset);

}
