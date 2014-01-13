#ifndef QHEXEDITPRIVATE_H
#define QHEXEDITPRIVATE_H

#include <QtCore>
#include <QtWidgets>
#include "qhexeditdata.h"

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
        void setBaseAddress(qint64 ba);
        void setCursorPos(qint64 pos);
        void setData(QHexEditData *hexeditdata);
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
        int visibleLinesCount();
        int wheelScrollLines();
        qint64 indexOf(QByteArray& ba, bool start);
        qint64 baseAddress();
        qint64 cursorPos();
        qint64 selectionStart();
        qint64 selectionEnd();
        qint64 visibleStartOffset();
        qint64 visibleEndOffset();
        QHexEditData *data();
        QColor& lineColor();
        QColor& addressForeColor();
        QColor& addressBackColor();
        QColor& alternateLineColor();
        QBrush& selectedCursorBrush();        

    private:
        void internalSetCursorPos(qint64 pos, int charidx);

    private:
        qint64 cursorPosFromPoint(const QPoint& pt, int* charindex);
        qint64 verticalSliderPosition64();
        QColor byteWeight(uchar b);
        bool isTextSelected();
        void removeSelectedText();
        void processDeleteEvents();
        void processBackspaceEvents();
        void processHexPart(int key);
        void processAsciiPart(int key);
        bool processMoveEvents(QKeyEvent* event);
        bool processSelectEvents(QKeyEvent* event);
        bool processTextInputEvents(QKeyEvent* event);
        bool processInsOvrEvents(QKeyEvent* event);
        bool processUndoRedo(QKeyEvent* event);
        bool processClipboardKeys(QKeyEvent* event);
        void setCursorPos(qint64 pos, int charidx);
        void updateCursorXY(qint64 pos, int charidx);
        void drawParts(QPainter& painter);
        void drawLine(QPainter& painter, QFontMetrics& fm, qint64 line, int y);
        void drawAddress(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void drawHexPart(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void drawAsciiPart(QPainter &painter, QFontMetrics &fm, qint64 line, int y);
        void setSelectionEnd(qint64 pos, int charidx);
        void ensureVisible();
        void adjust();

    protected:
        void paintEvent(QPaintEvent* pe);
        void mousePressEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);
        void resizeEvent(QResizeEvent*);

    private: /* Constants */
        static const int CURSOR_BLINK_INTERVAL;
        static const int BYTES_PER_LINE;

    signals:
        void positionChanged(qint64 offset);
        void selectionChanged(qint64 length);
        void verticalScrollBarValueChanged(int value);

    private:
        enum SelectedPart { AddressPart = 0, HexPart = 1, AsciiPart = 2 };
        enum InsertMode { Overwrite = 0, Insert = 1 };
        QMap<qint64, QColor> _highlightmap;
        QKeyEvent* _lastkeyevent;
        QHexEditData* _hexeditdata;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QTimer* _timBlink;
        QColor _selLineColor;
        QColor _addressforecolor;
        QColor _alternatelinecolor;
        QColor _addressbackcolor;
        QBrush _selcursorbrush;
        SelectedPart _selpart;
        InsertMode _insmode;
        qint64 _selectionstart;
        qint64 _selectionend;
        qint64 _cursorpos;
        qint64 _baseaddress;
        int _whellscrolllines;
        int _cursorX;
        int _cursorY;
        int _addressWidth;
        int _xposascii;
        int _xposhex;
        int _xPosend;
        int _charidx;
        int _charwidth;
        int _charheight;
        bool _readonly;
        bool _blink;

    private slots:
        void blinkCursor();
        void vScrollBarValueChanged(int);
        void hexEditDataChanged(qint64 offset, qint64, QHexEditData::ActionType reason);
};

#endif // QHEXEDITPRIVATE_H
