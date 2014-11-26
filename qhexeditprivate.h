#ifndef QHEXEDITPRIVATE_H
#define QHEXEDITPRIVATE_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexeditdata.h"
#include "qhexeditdatareader.h"
#include "qhexeditdatawriter.h"
#include "qhexedithighlighter.h"
#include "qhexeditcomments.h"

class QHexEditPrivate : public QWidget
{
    Q_OBJECT

    public:
        explicit QHexEditPrivate(QScrollArea* scrollarea, QScrollBar* vscrollbar, QWidget *parent = 0);
        void undo();
        void redo();
        void cut();
        void copy();
        void copyHex();
        void paste();
        void pasteHex();
        void setBaseAddress(qint64 ba);
        void setCursorPos(qint64 pos);
        void setData(QHexEditData *hexeditdata);
        void setReadOnly(bool b);
        void setAddressWidth(int w);
        void setWheelScrollLines(int c);
        void setSelection(qint64 start, qint64 end);
        void highlightForeground(qint64 start, qint64 end, const QColor& color);
        void highlightBackground(qint64 start, qint64 end, const QColor& color);
        void clearHighlight(qint64 start, qint64 end);
        void clearHighlight();
        void commentRange(qint64 start, qint64 end, const QString& comment);
        void uncommentRange(qint64 start, qint64 end);
        void clearComments();
        void setFont(const QFont& font);
        void setLineColor(const QColor& c);
        void setAddressForeColor(const QColor& c);
        void setAddressBackColor(const QColor& c);
        void setAlternateLineColor(const QColor& c);
        void setSelectedCursorBrush(const QBrush &b);
        void setVerticalScrollBarValue(int value);
        void scroll(QWheelEvent *event);
        bool readOnly();
        int addressWidth();
        int visibleLinesCount();
        int wheelScrollLines();
        qint64 indexOf(QByteArray& ba, qint64 start);
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
        void checkVisibleLines();

    private:
        qint64 cursorPosFromPoint(const QPoint& pt, int* charindex);
        qint64 verticalSliderPosition64();
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
        void colorize(uchar b, qint64 pos, QColor& bchex, QColor& fchex, QColor& bcascii, QColor& fcascii);
        void setCursorPos(qint64 pos, int charidx);
        void updateCursorXY(qint64 pos, int charidx);
        void drawParts(QPainter& painter);
        void drawLineBackground(QPainter& painter, qint64 line, qint64 linestart, int y);
        void drawLine(QPainter& painter, QFontMetrics& fm, qint64 line, int y);
        void drawAddress(QPainter &painter, QFontMetrics &fm, qint64 line, qint64 linestart, int y);
        void drawHex(QPainter &painter, QFontMetrics &fm, const QColor& bc, const QColor& fc, uchar b, qint64 i, int& x, int y);
        void drawAscii(QPainter &painter, QFontMetrics &fm, const QColor& bc, const QColor& fc, uchar b, int& x, int y);
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
        static QString UNPRINTABLE_CHAR;
        static const int CURSOR_BLINK_INTERVAL;
        static const qint64 BYTES_PER_LINE;

    signals:
        void visibleLinesChanged();
        void positionChanged(qint64 offset);
        void selectionChanged(qint64 length);
        void verticalScrollBarValueChanged(int value);

    private:
        enum SelectedPart { AddressPart = 0, HexPart = 1, AsciiPart = 2 };
        enum InsertMode { Overwrite = 0, Insert = 1 };
        QHexEditHighlighter* _highlighter;
        QHexEditComments* _comments;
        QKeyEvent* _lastkeyevent;
        QHexEditData* _hexeditdata;
        QHexEditDataReader* _reader;
        QHexEditDataWriter* _writer;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QTimer* _timBlink;
        QColor _sellinecolor;
        QColor _addressforecolor;
        QColor _alternatelinecolor;
        QColor _addressbackcolor;
        QBrush _selcursorbrush;
        SelectedPart _selpart;
        InsertMode _insmode;
        qint64 _selectionstart; /* Start of Selection (Inclusive)   */
        qint64 _selectionend;   /* End of Selection (NOT Inclusive) */
        qint64 _cursorpos;
        qint64 _baseaddress;
        qint64 _lastvisiblelines;
        qint64 _lastvscrollpos;
        int _whellscrolllines;
        int _cursorX;
        qint64 _cursorY;
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
        void onVScrollBarValueChanged(int);
        void hexEditDataChanged(qint64 offset, qint64, QHexEditData::ActionType reason);
};

#endif // QHEXEDITPRIVATE_H
