#include "qhexmetrics.h"

const sinteger_t QHexMetrics::BYTES_PER_LINE = 0x10;
const sinteger_t QHexMetrics::DEFAULT_ADDRESS_WIDTH = 8;

#define currentContainer static_cast<QWidget*>(this->parent())

QHexMetrics::QHexMetrics(QScrollBar *vscrollbar, QWidget *parent): QObject(parent), _vscrollbar(vscrollbar), _document(NULL)
{
    this->_addresswidth = 0;
    this->_charwidth = this->_charheight = 0;
    this->_xposascii = this->_xposhex = this->_xposend = 0;
}

QHexDocument *QHexMetrics::document() const
{
    return this->_document;
}

QScrollBar *QHexMetrics::verticalScrollBar() const
{
    return this->_vscrollbar;
}

QSize QHexMetrics::charSize() const
{
    return QSize(this->_charwidth, this->_charheight);
}

integer_t QHexMetrics::visibleStartOffset() const
{
    return qMin(static_cast<integer_t>(this->_vscrollbar->sliderPosition() * QHexMetrics::BYTES_PER_LINE), this->_document->length());
}

integer_t QHexMetrics::visibleEndOffset() const
{
    integer_t endoffset = this->_vscrollbar->sliderPosition() + (this->visibleLines() * QHexMetrics::BYTES_PER_LINE);

    if(endoffset)
        endoffset--;

    return qMin(endoffset, this->_document->length());
}

integer_t QHexMetrics::visibleLines() const
{
    return currentContainer->height() / this->_charheight;
}

integer_t QHexMetrics::currentLine() const
{
    QHexCursor* cursor = this->_document->cursor();
    return this->_vscrollbar->sliderPosition() + (cursor->cursorY() / this->_charheight);
}

integer_t QHexMetrics::addressWidth() const
{
    return this->_addresswidth;
}

integer_t QHexMetrics::charWidth() const
{
    return this->_charwidth;
}

integer_t QHexMetrics::charHeight() const
{
    return this->_charheight;
}

sinteger_t QHexMetrics::xPosAscii() const
{
    return this->_xposascii;
}

sinteger_t QHexMetrics::xPosHex() const
{
    return this->_xposhex;
}

sinteger_t QHexMetrics::xPosEnd() const
{
    return this->_xposend;
}

void QHexMetrics::calculate(QHexDocument *document, const QFontMetrics &fm)
{
    this->_document = document;
    this->calculate(fm);
}

void QHexMetrics::calculate(const QFontMetrics &fm)
{
    this->calculateAddressWidth();

    this->_charwidth = fm.width(" ");
    this->_charheight = fm.height();

    this->_xposhex = this->_charwidth * (this->_addresswidth + 1);
    this->_xposascii = this->_xposhex + (this->_charwidth * (QHexMetrics::BYTES_PER_LINE * 3));
    this->_xposend = this->_xposascii + (this->_charwidth * QHexMetrics::BYTES_PER_LINE);

    if(this->_document)
    {
        integer_t totlines = this->_document->length() / QHexMetrics::BYTES_PER_LINE;
        integer_t vislines = this->visibleLines();

        if(totlines > vislines)
        {
            this->_vscrollbar->setRange(0, (totlines - vislines) + 1);
            this->_vscrollbar->setSingleStep(1);
            this->_vscrollbar->setPageStep(vislines);
            this->_vscrollbar->show();
        }
        else
            this->_vscrollbar->hide();

        currentContainer->setMinimumWidth(this->_xposend);
    }
    else
    {
        this->_vscrollbar->hide();
        currentContainer->setMinimumWidth(0);
    }
}

bool QHexMetrics::ensureVisible()
{
    if(!this->_document)
        return false;

    sinteger_t currline = this->currentLine(), vislines = this->visibleLines();

    if(currline < this->_vscrollbar->sliderPosition() || currline > (this->_vscrollbar->sliderPosition() + vislines))
    {
        this->_vscrollbar->setValue(qMax(currline - (vislines / 2), sinteger_t(0)));
        return true;
    }

    return false;
}

void QHexMetrics::calculateAddressWidth()
{
    if(!this->_document)
    {
        this->_addresswidth = QHexMetrics::DEFAULT_ADDRESS_WIDTH;
        return;
    }

    QString addr = QString("%1").arg(this->_document->length() / QHexMetrics::BYTES_PER_LINE);

    if(this->_addresswidth >= addr.length())
        return;

    this->_addresswidth = addr.length();
}
