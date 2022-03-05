#pragma once

#include <QTextCharFormat>
#include <QObject>

class QHexView;

class QHexRenderDelegate: public QObject
{
    Q_OBJECT

    public:
        explicit QHexRenderDelegate(QObject* parent = nullptr);
        virtual ~QHexRenderDelegate() = default;
        virtual bool render(quint64 offset, quint8 b, QTextCharFormat& outcf, const QHexView* hexview) const = 0;
};
