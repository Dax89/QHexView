#ifndef QHEXMETRICS_H
#define QHEXMETRICS_H

#include <QScrollBar>
#include <QFontMetrics>
#include <QFont>
#include "../document/qhexdocument.h"

class QHexMetrics : public QObject
{
    Q_OBJECT

    public:
        explicit QHexMetrics(QScrollBar* vscrollbar, QWidget *parent = 0);
        QHexDocument* document() const;
        QScrollBar* verticalScrollBar() const;
        QSize charSize() const;
        integer_t visibleStartOffset() const;
        integer_t visibleEndOffset() const;
        integer_t visibleLines() const;
        integer_t currentLine() const;
        integer_t addressWidth() const;
        integer_t charWidth() const;
        integer_t charHeight() const;
        sinteger_t xPosAscii() const;
        sinteger_t xPosHex() const;
        sinteger_t xPosEnd() const;
        void calculate(QHexDocument* document, const QFontMetrics& fm);
        void calculate(const QFontMetrics& fm);
        bool ensureVisible();

    private:
        void calculateAddressWidth();

    public:
        static const sinteger_t BYTES_PER_LINE;

    private:
        static const sinteger_t DEFAULT_ADDRESS_WIDTH;
        QScrollBar* _vscrollbar;
        QHexDocument* _document;
        sinteger_t _addresswidth;
        sinteger_t _charwidth, _charheight;
        sinteger_t _xposascii, _xposhex, _xposend;
};

#endif // QHEXMETRICS_H
