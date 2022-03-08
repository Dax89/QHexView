#include "qhexdelegate.h"

QHexDelegate::QHexDelegate(QObject* parent): QObject{parent} { }

QString QHexDelegate::addressHeader(const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    return QString();
}

QString QHexDelegate::asciiHeader(const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    return QString();
}

bool QHexDelegate::render(quint64 offset, quint8 b, QTextCharFormat& outcf, const QHexView* hexview) const
{
    Q_UNUSED(offset);
    Q_UNUSED(b);
    Q_UNUSED(outcf);
    Q_UNUSED(hexview);

    return false;
}
