#include "qhexeditprivate.h"
#include "paint/qhexpainter.h"
#include <QKeyEvent>
#include <QToolTip>

const integer_t QHexEditPrivate::WHELL_SCROLL_LINES = 5;

QHexEditPrivate::QHexEditPrivate(QScrollArea *scrollarea, QScrollBar *vscrollbar, QWidget *parent): QWidget(parent), _document(NULL)
{
    this->_theme = new QHexTheme(this);
    this->_theme->setBaseColor(this->palette().color(QPalette::Base));

    this->_metrics = new QHexMetrics(vscrollbar, this);

    this->_scrollarea = scrollarea;
    this->_vscrollbar = vscrollbar;
    this->_readonly = false;

    connect(this->_vscrollbar, &QScrollBar::valueChanged, [this] { this->update(); });
    connect(this->_vscrollbar, &QScrollBar::valueChanged, this, &QHexEditPrivate::verticalScroll);

    this->setMouseTracking(true);
    this->setCursor(Qt::IBeamCursor);
    this->setFocusPolicy(Qt::StrongFocus);

    this->_metrics->calculate(this->fontMetrics());
}

QHexDocument *QHexEditPrivate::document() const
{
    return this->_document;
}

QHexMetrics *QHexEditPrivate::metrics() const
{
    return this->_metrics;
}

bool QHexEditPrivate::readOnly() const
{
    return this->_readonly;
}

void QHexEditPrivate::setDocument(QHexDocument *document)
{
    if(this->_document)
    {
        disconnect(this->_document, &QHexDocument::canUndoChanged, this, 0);
        disconnect(this->_document, &QHexDocument::canRedoChanged, this, 0);
        disconnect(this->_document, &QHexDocument::documentChanged, this, 0);
        disconnect(this->_document, &QHexDocument::baseAddressChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::blinkChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::positionChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::offsetChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::selectionChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::selectedPartChanged, this, 0);
        disconnect(this->_document->cursor(), &QHexCursor::insertionModeChanged, this, 0);
    }

    this->_document = document;
    this->_metrics->calculate(document, this->fontMetrics());
    document->cursor()->setPosition(this->_metrics->xPosHex(), 0);

    connect(document, &QHexDocument::documentChanged, [this]() { this->update(); });
    connect(document->cursor(), &QHexCursor::selectionChanged, [this]() { this->update(); });

    connect(document->cursor(), &QHexCursor::offsetChanged, [this]() {
        QHexCursor* cursor = this->_document->cursor();
        this->updateCaret(cursor->offset(), cursor->nibbleIndex());

        if(!this->_metrics->ensureVisible())
            this->update();
    });

    connect(document->cursor(), &QHexCursor::blinkChanged, [this]() {
        QHexCursor* cursor = this->_document->cursor();
        this->update(QRect(cursor->position(), this->_metrics->charSize()));
    });

    this->update();
}

void QHexEditPrivate::setReadOnly(bool b)
{
    this->_readonly = b;
}

void QHexEditPrivate::scroll(QWheelEvent *event)
{
    this->wheelEvent(event);
}

void QHexEditPrivate::processDeleteEvents()
{
    QHexCursor* cursor = this->_document->cursor();

    if(cursor->removeSelection())
        return;

    if(cursor->isOverwriteMode())
    {
        if(cursor->isHexPartSelected())
        {
            uchar hexval = this->_document->at(cursor->offset());

            if(cursor->nibbleIndex() == 1) // Change Low Part
                hexval = (hexval & 0xF0);
            else // Change High Part
                hexval = (hexval & 0x0F);

            this->_document->replace(cursor->offset(), hexval);
        }
        else
            this->_document->replace(cursor->offset(), 0x00);
    }
    else
        this->_document->remove(cursor->offset(), 1);
}

void QHexEditPrivate::processBackspaceEvents()
{
    QHexCursor* cursor = this->_document->cursor();

    if(cursor->removeSelection())
        return;

    integer_t pos = cursor->offset() > 0 ? cursor->offset() - 1 : 0;

    if(cursor->isOverwriteMode())
        this->_document->replace(pos, 0x00);
    else
        this->_document->remove(pos, 1);
}

void QHexEditPrivate::processHexPart(int key)
{
    QHexCursor* cursor = this->_document->cursor();
    uchar val = static_cast<uchar>(QString(key).toUInt(NULL, 16));

    cursor->removeSelection();

    if((cursor->isInsertMode()) && !cursor->nibbleIndex()) // Insert a new byte
    {
        this->_document->insert(cursor->offset(), val << 4); // X0 byte
        cursor->moveOffset(1, true);
        return;
    }

    if((cursor->isOverwriteMode() && !this->_document->isEmpty()) || cursor->nibbleIndex()) // Override mode, or update low nibble
    {
        uchar hexval = this->_document->at(cursor->offset());

        if(cursor->nibbleIndex() == 1) // Change Low Part
            hexval = (hexval & 0xF0) + val;
        else // Change High Part
            hexval = (hexval & 0x0F) + (val << 4);

        this->_document->replace(cursor->offset(), hexval);
        cursor->moveOffset(1, true);
    }
}

void QHexEditPrivate::processAsciiPart(int key)
{
    QHexCursor* cursor = this->_document->cursor();
    cursor->removeSelection();

    if(cursor->isInsertMode())
        this->_document->insert(cursor->offset(), static_cast<uchar>(key));
    else if(cursor->isOverwriteMode() && !this->_document->isEmpty())
        this->_document->replace(cursor->offset(), static_cast<uchar>(key));

    cursor->moveOffset(1);
}

bool QHexEditPrivate::processMoveEvents(QKeyEvent *event)
{
    QHexCursor* cursor = this->_document->cursor();

    if(event->matches(QKeySequence::MoveToNextChar))
        cursor->moveOffset(1, true);
    else if(event->matches(QKeySequence::MoveToPreviousChar))
        cursor->moveOffset(-1, true);
    else if(event->matches(QKeySequence::MoveToNextLine))
        cursor->moveOffset(QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::MoveToPreviousLine))
        cursor->moveOffset(-QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::MoveToStartOfLine))
        cursor->moveOffset(-(cursor->offset() % QHexMetrics::BYTES_PER_LINE));
    else if(event->matches(QKeySequence::MoveToEndOfLine))
        cursor->setOffset(cursor->offset() | (QHexMetrics::BYTES_PER_LINE - 1));
    else if(event->matches(QKeySequence::MoveToNextPage))
        cursor->moveOffset((this->_metrics->visibleLines() - 1) * QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::MoveToPreviousPage))
        cursor->moveOffset(-(this->_metrics->visibleLines() - 1) * QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::MoveToStartOfDocument))
        cursor->setOffset(0);
    else if(event->matches(QKeySequence::MoveToEndOfDocument))
        cursor->setOffset(this->_document->length());
    else
        return false;

    return true;
}

bool QHexEditPrivate::processSelectEvents(QKeyEvent *event)
{
    QHexCursor* cursor = this->_document->cursor();

    if(event->matches(QKeySequence::SelectNextChar))
        cursor->moveSelection(1);
    else if(event->matches(QKeySequence::SelectPreviousChar))
        cursor->moveSelection(-1);
    else if(event->matches(QKeySequence::SelectNextLine))
        cursor->moveSelection(QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::SelectPreviousLine))
        cursor->moveSelection(-QHexMetrics::BYTES_PER_LINE);
    else if(event->matches(QKeySequence::SelectStartOfDocument))
        cursor->selectStart();
    else if(event->matches(QKeySequence::SelectEndOfDocument))
        cursor->selectEnd();
    else if(event->matches(QKeySequence::SelectAll))
        cursor->selectAll();
    else
        return false;

    return true;
}

bool QHexEditPrivate::processTextInputEvents(QKeyEvent *event)
{
    if(!this->_document || (event->modifiers() & Qt::ControlModifier))
        return false;

    QHexCursor* cursor = this->_document->cursor();
    int key = static_cast<int>(event->text()[0].toLatin1());

    if(cursor->isHexPartSelected() && ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f'))) /* Check if is a Hex Char */
        this->processHexPart(key);
    else if(cursor->isAsciiPartSelected() && (key >= 0x20 && key <= 0x7E)) /* Check if is a Printable Char */
        this->processAsciiPart(key);
    else if(event->key() == Qt::Key_Delete)
        this->processDeleteEvents();
    else if((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
        this->processBackspaceEvents();
    else
        return false;

    return true;
}

bool QHexEditPrivate::processInsOvrEvents(QKeyEvent *event)
{
    if((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        QHexCursor* cursor = this->_document->cursor();
        cursor->switchMode();
        cursor->blink(true);
        return true;
    }

    return false;
}

bool QHexEditPrivate::processUndoRedo(QKeyEvent *event)
{
    if(event->matches(QKeySequence::Undo))
        this->_document->undo();
    else if(event->matches(QKeySequence::Redo))
        this->_document->redo();
    else
        return false;

    return true;
}

bool QHexEditPrivate::processClipboardKeys(QKeyEvent *event)
{
    if(event->matches(QKeySequence::Cut))
        this->_document->cut();
    else if(event->matches(QKeySequence::Copy))
        this->_document->copy();
    else if(event->matches(QKeySequence::Paste))
        this->_document->paste();
    else
        return false;

    return true;
}

void QHexEditPrivate::updateCaret(integer_t offset, integer_t nibbleindex)
{
    if(!this->_document)
        return;

    QHexCursor* cursor = this->_document->cursor();
    sinteger_t cursorx = 0, cursory = ((static_cast<sinteger_t>(offset) - static_cast<sinteger_t>(this->_vscrollbar->sliderPosition() * QHexMetrics::BYTES_PER_LINE)) /
                                       QHexMetrics::BYTES_PER_LINE) * this->_metrics->charHeight();

    if(cursor->isAddressPartSelected() || cursor->isHexPartSelected())
    {
        sinteger_t x = offset % QHexMetrics::BYTES_PER_LINE;
        cursorx = x * (3 * this->_metrics->charWidth()) + this->_metrics->xPosHex();

        if(nibbleindex)
            cursorx += this->_metrics->charWidth();
    }
    else // AsciiPart
    {
        sinteger_t x = offset % QHexMetrics::BYTES_PER_LINE;
        cursorx = x * this->_metrics->charWidth() + this->_metrics->xPosAscii();
    }

    cursor->blink(false);
    cursor->setPosition(cursorx, cursory);
    cursor->blink(true);
}

integer_t QHexEditPrivate::offsetFromPoint(const QPoint &pt, integer_t *nibbleindex) const
{
    if(!this->_document)
        return 0;

    QHexCursor* cursor = this->_document->cursor();
    integer_t y = (this->_vscrollbar->sliderPosition() + (pt.y() / this->_metrics->charHeight())) * QHexMetrics::BYTES_PER_LINE, x = 0;

    if(nibbleindex)
        *nibbleindex = 0;

    if(cursor->isHexPartSelected())
    {
        x = (pt.x() - this->_metrics->xPosHex()) / this->_metrics->charWidth();

        if(nibbleindex && (x % 3) != 0) // Is second nibble selected?
            *nibbleindex = 1;

        x = x / 3;
    }
    else if(cursor->isAsciiPartSelected())
        x = ((pt.x() - this->_metrics->xPosAscii()) / this->_metrics->charWidth());

    return y + x;
}

void QHexEditPrivate::toggleComment(const QPoint& pos)
{
    integer_t offset = this->offsetFromPoint(pos);
    QString comment = this->_document->metadata()->commentString(offset);

    if(comment.isEmpty())
        QToolTip::hideText();
    else
        QToolTip::showText(this->mapToGlobal(pos), comment, this);
}

void QHexEditPrivate::paintEvent(QPaintEvent* pe)
{
    QHexPainter hexpainter(this->_metrics, this);
    hexpainter.paint(pe, this->_theme);
}

void QHexEditPrivate::mousePressEvent(QMouseEvent* event)
{
    if(!this->_document || !(event->buttons() & Qt::LeftButton))
        return;

    QPoint pos = event->pos();

    if(!pos.x() && !pos.y())
        return;

    QHexMetrics* metrics = this->_metrics;
    QHexCursor* cursor = this->_document->cursor();
    integer_t span = metrics->charHeight() / 2;

    if(pos.x() < static_cast<sinteger_t>(metrics->xPosHex() - span))
        cursor->setSelectedPart(QHexCursor::AddressPart);
    else if(pos.x() >= static_cast<sinteger_t>(metrics->xPosAscii() - span))
        cursor->setSelectedPart(QHexCursor::AsciiPart);
    else
        cursor->setSelectedPart(QHexCursor::HexPart);

    integer_t nibbleidx = 0, offset = this->offsetFromPoint(pos, &nibbleidx);

    if(event->modifiers() == Qt::ShiftModifier)
        cursor->setSelectionEnd(offset);
    else
        cursor->setOffset(offset, nibbleidx);
}

void QHexEditPrivate::mouseMoveEvent(QMouseEvent* event)
{
    if(!this->_document)
        return;

    if(event->buttons() & Qt::LeftButton)
    {
        QHexCursor* cursor = this->_document->cursor();
        QPoint pos = event->pos();

        if(!pos.x() && !pos.y())
            return;

        integer_t offset = this->offsetFromPoint(pos);
        cursor->setSelectionEnd(offset);
    }
    else if(event->buttons() == Qt::NoButton)
        this->toggleComment(event->pos());
}

void QHexEditPrivate::wheelEvent(QWheelEvent *event)
{
    if(!this->_document || !this->_document->length() || (event->orientation() != Qt::Vertical))
    {
        event->ignore();
        return;
    }

    QHexCursor* cursor = this->_document->cursor();
    sinteger_t numdegrees = event->delta() / 8, numsteps = numdegrees / 15;
    sinteger_t maxlines = this->_document->length() / QHexMetrics::BYTES_PER_LINE;
    sinteger_t pos = this->_vscrollbar->sliderPosition() - (numsteps * QHexEditPrivate::WHELL_SCROLL_LINES);

    if(pos < 0)
        pos = 0;
    else if(pos > maxlines)
        pos = maxlines;

    this->_vscrollbar->setSliderPosition(pos);
    this->updateCaret(cursor->offset(), cursor->nibbleIndex());
    this->update();
    event->accept();
}

void QHexEditPrivate::keyPressEvent(QKeyEvent* event)
{
    if(!this->_document)
        return;

    if(this->processMoveEvents(event))
        return;

    if(this->processSelectEvents(event))
        return;

    if(this->processInsOvrEvents(event))
        return;

    if(this->processUndoRedo(event))
        return;

    if(this->processClipboardKeys(event))
        return;

    if(!this->_readonly)
        this->processTextInputEvents(event);
}

void QHexEditPrivate::resizeEvent(QResizeEvent* e)
{
    this->_metrics->calculate(this->fontMetrics()); // Update ScrollBars
    QWidget::resizeEvent(e);
}
