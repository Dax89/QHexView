#ifndef QHEXEDITPRIVATE_H
#define QHEXEDITPRIVATE_H

#include <QtCore>
#include <QtWidgets>
#include "qhexeditdatamanager.h"

/* TODO: Improve Drawing Routines */

class QHexEditPrivate : public QWidget
{
    Q_OBJECT

    public:
        explicit QHexEditPrivate(QScrollArea* scrollarea, QScrollBar* vscrollbar, QWidget *parent = 0);
        void undo();
        void redo();
        void cut();
        void copy();
        void paste();
        void doAnd(qint64 start, qint64 end, uchar value);
        void doOr(qint64 start, qint64 end, uchar value);
        void doXor(qint64 start, qint64 end, uchar value);
        void doMod(qint64 start, qint64 end, uchar value);
        void doNot(qint64 start, qint64 end);
        void setCursorPos(qint64 pos);
        void setData(QHexEditData* hexeditdata);
        void setReadOnly(bool b);
        void setAddressWidth(int w);
        void setWheelScrollLines(int c);
        void setSelection(qint64 start, qint64 end);
        void setRangeColor(qint64 start, qint64 end, QColor color);
        void removeRangeColor(qint64 start, qint64 end);
        void resetRangeColor();
        void setFont(const QFont& font);
        void setLineColor(const QColor& c);
        void setAddressForeColor(const QColor& c);
        void setAddressBackColor(const QColor& c);
        void setAlternateLineColor(const QColor& c);
        void setSelectedCursorBrush(const QBrush &b);
        void setVerticalScrollBarValue(int value);
        bool readOnly();
        int addressWidth();
        int wheelScrollLines();
        qint64 indexOf(QByteArray& ba, bool start);
        qint64 cursorPos();
        qint64 selectionStart();
        qint64 selectionEnd();
        QHexEditData* data();
        QColor& lineColor();
        QColor& addressForeColor();
        QColor& addressBackColor();
        QColor& alternateLineColor();
        QBrush& selectedCursorBrush();

    private:
        QColor byteWeight(uchar b);
        void setCursorPos(qint64 pos, int charidx);
        void setCursorXY(qint64 pos, int charidx);
        void drawLine(QPainter& painter, QFontMetrics& fm, qint64 line, int y);
        void drawAddress(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void drawHexPart(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void drawAsciiPart(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void setSelectionEnd(qint64 pos, int charidx);
        void ensureVisible();
        void adjust();
        qint64 cursorPosFromPoint(const QPoint& pt, int* charindex);
        qint64 verticalSliderPosition64();

    protected:
        void paintEvent(QPaintEvent* pe);
        void mousePressEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);
        void resizeEvent(QResizeEvent*);

    private: /* Constants */
        static const int CURSOR_BLINK_INTERVAL;

    signals:
        void positionChanged(qint64 offset);
        void selectionChanged(qint64 length);
        void bytesChanged(qint64 pos);
        void verticalScrollBarValueChanged(int value);

    private:
        enum SelectedPart
        {
            AddressPart = 0,
            HexPart     = 1,
            AsciiPart   = 2
        };

        QMap<qint64, QColor> _highlightmap;
        QHexEditDataManager* _hexeditdatamanager;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QTimer* _timBlink;
        QColor _selLineColor;
        QColor _addressforecolor;
        QColor _alternatelinecolor;
        QColor _addressbackcolor;
        QBrush _selcursorbrush;
        SelectedPart _selpart;
        int _whellscrolllines;
        int _cursorX;
        int _cursorY;
        int _addressWidth;
        int _xposascii;
        int _xposhex;
        int _xPosend;
        int _charwidth;
        int _charheight;
        bool _readonly;
        bool _blink;

    private slots:
        void blinkCursor();
        void vScrollBarValueChanged(int);
        void hexEditDataChanged(qint64, qint64);
};

#endif // QHEXEDITPRIVATE_H
