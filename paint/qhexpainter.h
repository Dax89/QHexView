#ifndef QHEXPAINTER_H
#define QHEXPAINTER_H

#include <QScrollBar>
#include "../document/qhexdocument.h"
#include "../document/qhextheme.h"
#include "qhexmetrics.h"

class QHexPainter : public QObject
{
    Q_OBJECT

    public:
        explicit QHexPainter(QHexMetrics* metrics, QWidget *parent = 0);
        void paint(QPaintEvent* e, QHexTheme* theme);

    private:
        void drawCursor(QPainter *painter);
        void drawBackground(QPainter *painter);
        void drawLines(QPaintEvent* e, QPainter *painter, QHexTheme* theme);
        void drawLine(QPainter* painter, QHexTheme* theme, integer_t line, integer_t y);
        void drawLineBackground(QPainter *painter, QHexTheme* theme, integer_t line, integer_t linestart, integer_t y);
        void drawAddress(QPainter* painter, QHexTheme *theme, integer_t line, integer_t linestart, integer_t y);
        void drawHex(QPainter* painter, uchar b, sinteger_t i, integer_t offset, integer_t& x, integer_t y);
        void drawAscii(QPainter* painter, uchar b, integer_t offset, integer_t &x, integer_t y);
        void colorize(QPainter* painter, integer_t offset, uchar b);
        bool applyMetadata(QPainter* painter, integer_t offset);
        bool mark(QPainter* painter, const QRect& r, integer_t offset, QHexCursor::SelectedPart part);

    private:
        static QString UNPRINTABLE_CHAR;
        QHexMetrics* _metrics;
        QHexDocument* _document;
        QScrollBar* _vscrollbar;
        QFont _boldfont;
};

#endif // QHEXPAINTER_H
