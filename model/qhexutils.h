#pragma once

#include <QByteArray>
#include <QVariant>
#include <QString>
#include <utility>

struct QHexOptions;
class QHexView;

enum class HexFindMode { Text, Hex, Int, Float };
enum class HexFindDirection { All, Forward, Backward };
enum class HexArea { Header, Address, Hex, Ascii, Extra };

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
std::pair<qint64, qint64> find(const QHexView* hexview, QVariant value, HexFindMode mode = HexFindMode::Text, HexFindDirection fd = HexFindDirection::Forward);
HexPosition offsetToPosition(const QHexOptions* options, qint64 offset);

}
