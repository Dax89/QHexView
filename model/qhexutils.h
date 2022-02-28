#pragma once

#include <QByteArray>
#include <QString>

namespace QHexUtils {

QByteArray toHex(const QByteArray& ba, char sep);
QByteArray toHex(const QByteArray& ba);

}
