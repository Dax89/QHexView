#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexeditprivate.h"
#include "qhexeditdatareader.h"
#include "qhexeditdatawriter.h"
#include "qhexeditdatadevice.h"

class QHexEdit : public QFrame
{
    Q_OBJECT

    public:
        explicit QHexEdit(QWidget *parent = 0);
        bool readOnly();
        int addressWidth();
        int visibleLinesCount();
        QHexEditData *data();
        qint64 indexOf(QByteArray& ba, qint64 start = 0);
        qint64 baseAddress();
        qint64 cursorPos();
        qint64 selectionStart();
        qint64 selectionEnd();
        qint64 selectionLength();
        qint64 visibleStartOffset();
        qint64 visibleEndOffset();
        void setFont(const QFont &f);
        void setBaseAddress(qint64 ba);

    signals:
        void visibleLinesChanged();
        void positionChanged(qint64 offset);
        void selectionChanged(qint64 length);
        void bytesChanged(qint64 pos);
        void verticalScrollBarValueChanged(int value);

    public slots:
        void undo();
        void redo();
        void cut();
        void copy();
        void copyHex();
        void paste();
        void pasteHex();
        void selectAll();
        void setReadOnly(bool b);
        void setCursorPos(qint64 pos);
        void setData(QHexEditData *hexeditdata);
        void selectPos(qint64 pos);
        void setSelection(qint64 start, qint64 end);
        void setSelectionLength(qint64 start, qint64 length);
        void highlightBackground(qint64 start, qint64 end, const QColor& color);
        void highlightForeground(qint64 start, qint64 end, const QColor& color);
        void clearHighlight(qint64 start, qint64 end);
        void clearHighlight();
        void commentRange(qint64 start, qint64 end, const QString& comment);
        void uncommentRange(qint64 start, qint64 end);
        void clearComments();
        void setVerticalScrollBarValue(int value);
        void scroll(QWheelEvent *event);

    private:
        QHexEditPrivate* _hexedit_p;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QHBoxLayout* _hlayout;
};

#endif // QHEXEDIT_H
