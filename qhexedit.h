#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QtCore>
#include <QtWidgets>
#include "qhexeditprivate.h"

class QHexEdit : public QFrame
{
    Q_OBJECT

    public:
        explicit QHexEdit(QWidget *parent = 0);
        bool readOnly();
        int addressWidth();
        int visibleLinesCount();
        QHexEditData* data();
        qint64 indexOf(QByteArray& ba, qint64 start = 0);
        qint64 cursorPos();
        qint64 selectionStart();
        qint64 selectionEnd();
        void setFont(const QFont &f);
        qint64 visibleStartOffset();
        qint64 visibleEndOffset();

    signals:
        void positionChanged(qint64 offset);
        void selectionChanged(qint64 length);
        void bytesChanged(qint64 pos);
        void verticalScrollBarValueChanged(int value);

    public slots:
        void undo();
        void redo();
        void cut();
        void copy();
        void paste();
        void selectAll();
        void doAnd(qint64 start, qint64 end, uchar value);
        void doOr(qint64 start, qint64 end, uchar value);
        void doXor(qint64 start, qint64 end, uchar value);
        void doMod(qint64 start, qint64 end, uchar value);
        void doNot(qint64 start, qint64 end);
        void setReadOnly(bool b);
        void setCursorPos(qint64 pos);
        void setData(QHexEditData* hexeditdata);
        void setSelection(qint64 start, qint64 end);
        void setRangeColor(qint64 start, qint64 end, QColor color);
        void removeRangeColor(qint64 start, qint64 end);
        void resetRangeColor();
        void setVerticalScrollBarValue(int value);

    private:
        QHexEditPrivate* _hexedit_p;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QHBoxLayout* _hlayout;
};

#endif // QHEXEDIT_H
