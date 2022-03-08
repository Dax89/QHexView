#pragma once

#include <QTextCharFormat>
#include <QObject>

class QHexView;

class QHexDelegate: public QObject
{
    Q_OBJECT

    public:
        explicit QHexDelegate(QObject* parent = nullptr);
        virtual ~QHexDelegate() = default;
        virtual bool render(quint64 offset, quint8 b, QTextCharFormat& outcf, const QHexView* hexview) const;
};
