#include "qhexcursor.h"
#include "qhexdocument.h"
#include <QWidget>
#include <QPoint>

#define currentDocument static_cast<QHexDocument*>(this->parent())
//#define currentContainer static_cast<QWidget*>(currentDocument->parent())

const int QHexCursor::CURSOR_BLINK_INTERVAL = 500; // 5ms

QHexCursor::QHexCursor(QObject *parent) : QObject(parent), _selectedpart(QHexCursor::HexPart), _insertionmode(QHexCursor::OverwriteMode)
{
    this->_cursorx = this->_cursory = 0;
    this->_selectionstart = this->_offset = this->_nibbleindex = 0;
    this->_timerid = this->startTimer(QHexCursor::CURSOR_BLINK_INTERVAL);
}

QHexCursor::~QHexCursor()
{
    this->killTimer(this->_timerid);
}

QPoint QHexCursor::position() const
{
    return QPoint(this->_cursorx, this->_cursory);
}

sinteger_t QHexCursor::cursorX() const
{
    return this->_cursorx;
}

sinteger_t QHexCursor::cursorY() const
{
    return this->_cursory;
}

integer_t QHexCursor::offset() const
{
    return this->_offset;
}

integer_t QHexCursor::nibbleIndex() const
{
    return this->_nibbleindex;
}

integer_t QHexCursor::selectionStart() const
{
    return qMin(this->_offset, this->_selectionstart);
}

integer_t QHexCursor::selectionEnd() const
{
    return qMax(this->_offset, this->_selectionstart);
}

integer_t QHexCursor::selectionLength() const
{
    return this->selectionEnd() - this->selectionStart();
}

QHexCursor::SelectedPart QHexCursor::selectedPart() const
{
    return this->_selectedpart;
}

QHexCursor::InsertionMode QHexCursor::insertionMode() const
{
    return this->_insertionmode;
}

bool QHexCursor::isAddressPartSelected() const
{
    return this->_selectedpart == QHexCursor::AddressPart;
}

bool QHexCursor::isHexPartSelected() const
{
    return this->_selectedpart == QHexCursor::HexPart;
}

bool QHexCursor::isAsciiPartSelected() const
{
    return this->_selectedpart == QHexCursor::AsciiPart;
}

bool QHexCursor::isInsertMode() const
{
    return this->_insertionmode == QHexCursor::InsertMode;
}

bool QHexCursor::isOverwriteMode() const
{
    return this->_insertionmode == QHexCursor::OverwriteMode;
}

bool QHexCursor::hasSelection() const
{
    return this->_offset != this->_selectionstart;
}

bool QHexCursor::blinking() const
{
    return this->_blink;
}

bool QHexCursor::isSelected(integer_t offset) const
{
    if(this->_offset == this->_selectionstart)
        return false;

    return (offset >= this->selectionStart()) && (offset < this->selectionEnd());
}

void QHexCursor::selectStart()
{
    this->setOffset(0);
}

void QHexCursor::selectEnd()
{
    this->setOffset(currentDocument->length());
}

void QHexCursor::selectAll()
{
    this->setSelection(0, currentDocument->length());
}

void QHexCursor::setPosition(sinteger_t x, sinteger_t y)
{
    if((this->_cursorx == x) && (this->_cursory == y))
        return;

    this->_cursorx = x;
    this->_cursory = y;
    emit positionChanged();
}

void QHexCursor::setOffset(integer_t offset)
{
    this->setOffset(offset, 0);
}

void QHexCursor::setOffset(integer_t offset, integer_t nibbleindex)
{
    offset = qMin(offset, currentDocument->length()); // Check EOF
    this->_selectionstart = offset;
    this->_nibbleindex = nibbleindex;

    this->setSelectionEnd(offset);
}

void QHexCursor::setSelectionEnd(integer_t offset)
{
    offset = qMin(offset, currentDocument->length()); // Check EOF
    this->_offset = offset;

    emit selectionChanged();
    emit offsetChanged();
}

void QHexCursor::setSelection(integer_t startoffset, integer_t endoffset)
{
    this->setOffset(startoffset);
    this->setSelectionEnd(endoffset);
}

void QHexCursor::setSelectionRange(integer_t startoffset, integer_t length)
{
    this->setSelection(startoffset, startoffset + length);
}

void QHexCursor::setSelectedPart(QHexCursor::SelectedPart sp)
{
    if(sp == this->_selectedpart)
        return;

    this->_selectedpart = sp;
    emit selectedPartChanged();
}

void QHexCursor::clearSelection()
{
    if(this->_selectionstart == this->_offset)
        return;

    this->_selectionstart = this->_offset;
    emit selectionChanged();
}

bool QHexCursor::removeSelection()
{
    if(!this->hasSelection())
        return false;

    currentDocument->remove(this->selectionStart(), this->selectionLength());
    this->clearSelection();
    return true;
}

void QHexCursor::moveOffset(sinteger_t c, bool bynibble)
{
    if(!c)
        return;

    if(qAbs(c) > 1)
        bynibble = false;

    integer_t nindex = 0;

    if(bynibble)
    {
        if(!this->_nibbleindex)
        {
            if(c > 0)
                c = 0;

            nindex = 1;
        }
        else
        {
            if(c < 0)
                c = 0;

            nindex = 0;
        }
    }

    integer_t offset = this->_offset + c;

    if(offset >= currentDocument->length())
        offset = c > 0 ? currentDocument->length() : 0;

    this->setOffset(offset, nindex);
}

void QHexCursor::moveSelection(sinteger_t c)
{
    if(!c)
        return;

    integer_t offset = this->_offset + c;

    if(offset >= currentDocument->length())
        offset = c > 0 ? currentDocument->length() : 0;

    this->setSelectionEnd(offset);
}

void QHexCursor::blink(bool b)
{
    this->_blink = b;
    emit blinkChanged();
}

void QHexCursor::switchMode()
{
    if(this->_insertionmode == QHexCursor::OverwriteMode)
        this->_insertionmode = QHexCursor::InsertMode;
    else
        this->_insertionmode = QHexCursor::OverwriteMode;

    emit insertionModeChanged();
}

void QHexCursor::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    this->_blink = !this->_blink;
    emit blinkChanged();
}
