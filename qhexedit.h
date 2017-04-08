#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include "document/qhexdocument.h"
#include "paint/qhexmetrics.h"

class QHexEditPrivate;

class QHexEdit : public QFrame
{
    Q_OBJECT

    public:
        explicit QHexEdit(QWidget *parent = 0);
        QHexDocument *document() const;
        QHexMetrics *metrics() const;
        bool readOnly() const;

    public slots:
        void setReadOnly(bool b);
        void setDocument(QHexDocument *document);
        void scroll(QWheelEvent *event);

    signals:
        void verticalScroll(integer_t value);
        void visibleLinesChanged();

    private:
        QHexEditPrivate* _hexedit_p;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QHBoxLayout* _hlayout;
};

#endif // QHEXEDIT_H
