#include "qhexeditprivate.h"

const int QHexEditPrivate::CURSOR_BLINK_INTERVAL = 500; /* 0.5 sec */
const int QHexEditPrivate::BYTES_PER_LINE = 0x10;

QHexEditPrivate::QHexEditPrivate(QScrollArea *scrollarea, QScrollBar *vscrollbar, QWidget *parent): QWidget(parent)
{
    this->_hexeditdata = nullptr;
    this->_lastkeyevent = nullptr;
    this->_scrollarea = scrollarea;
    this->_vscrollbar = vscrollbar;
    this->_blink = false;
    this->_readonly = false;
    this->_baseaddress = this->_cursorX = this->_cursorY = this->_cursorpos = this->_selectionstart = this->_selectionend = this->_charidx = this->_charheight = this->_charwidth = 0;
    this->_selpart = QHexEditPrivate::HexPart;
    this->_insmode = QHexEditPrivate::Overwrite;

    connect(this->_vscrollbar, SIGNAL(valueChanged(int)), this, SLOT(vScrollBarValueChanged(int)));
    connect(this->_vscrollbar, SIGNAL(valueChanged(int)), this, SIGNAL(verticalScrollBarValueChanged(int)));

    QFont f("Courier", 10);
    f.setStyleHint(QFont::TypeWriter); /* Use monospace fonts! */

    this->setFont(f);
    this->setWheelScrollLines(5); /* By default: scroll 5 lines */
    this->setAddressWidth(8);
    this->setCursor(Qt::IBeamCursor);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setBackgroundRole(QPalette::Base);
    this->setSelectedCursorBrush(Qt::lightGray);
    this->setLineColor(QColor(0xFF, 0xFF, 0xA0));
    this->setAddressForeColor(Qt::darkBlue);
    this->setAddressBackColor(QColor(0xF0, 0xF0, 0xFE));
    this->setAlternateLineColor(QColor(0xF0, 0xF0, 0xFE));

    this->_timBlink = new QTimer();
    this->_timBlink->setInterval(QHexEditPrivate::CURSOR_BLINK_INTERVAL);

    connect(this->_timBlink, SIGNAL(timeout()), this, SLOT(blinkCursor()));
    this->_timBlink->start();
}

void QHexEditPrivate::undo()
{
    if(!this->_hexeditdata || !this->_hexeditdata->undoStack()->canUndo())
        return;

    this->_hexeditdata->undoStack()->undo();
}

void QHexEditPrivate::redo()
{
    if(!this->_hexeditdata || !this->_hexeditdata->undoStack()->canRedo())
        return;

    this->_hexeditdata->undoStack()->redo();
}

void QHexEditPrivate::cut()
{
    qint64 start = qMin(this->_selectionstart, this->_selectionend);
    qint64 end = qMax(this->_selectionstart, this->_selectionend);

    if(end - start)
    {
        QClipboard* cpbd = qApp->clipboard();
        QByteArray ba = this->_hexeditdata->read(start, end - start);

        cpbd->setText(QString(ba));
        this->_hexeditdata->remove(start, end - start);
        this->setCursorPos(start, 0);
        this->adjust();
        this->ensureVisible();
    }
}

void QHexEditPrivate::copy()
{
    qint64 start = qMin(this->_selectionstart, this->_selectionend);
    qint64 end = qMax(this->_selectionstart, this->_selectionend);

    if(end - start)
    {
        QClipboard* cpbd = qApp->clipboard();
        QByteArray ba = this->_hexeditdata->read(start, end - start);
        cpbd->setText(QString(ba));
    }
}

void QHexEditPrivate::paste()
{
    QClipboard* cpbd = qApp->clipboard();
    QString s = cpbd->text();

    if(!s.isNull())
    {
        QByteArray ba = s.toLatin1();
        qint64 start = qMin(this->_selectionstart, this->_selectionend);
        qint64 end = qMax(this->_selectionstart, this->_selectionend);

        if(end - start)
            this->_hexeditdata->replace(start, end - start, ba);
        else
            this->_hexeditdata->insert(start, ba);

        this->setCursorPos((start + s.length()), 0);
        this->adjust();
        this->ensureVisible();
    }
}

void QHexEditPrivate::setBaseAddress(qint64 ba)
{
    this->_baseaddress = ba;
}

void QHexEditPrivate::setData(QHexEditData *hexeditdata)
{
    if(this->_hexeditdata) /* Disconnect previous HexEditData Signals, if any */
        disconnect(this->_hexeditdata, SIGNAL(dataChanged(qint64,qint64,QHexEditData::ActionType)), this, SLOT(hexEditDataChanged(qint64,qint64,QHexEditData::ActionType)));

    if(hexeditdata)
    {
        this->_hexeditdata = hexeditdata;
        connect(hexeditdata, SIGNAL(dataChanged(qint64,qint64,QHexEditData::ActionType)), SLOT(hexEditDataChanged(qint64,qint64,QHexEditData::ActionType)));

        /* Check Max Address Width */
        QString addrString = QString("%1").arg(this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE);

        if(this->_addressWidth < addrString.length()) /* Adjust Address Width */
            this->_addressWidth = addrString.length();

        this->setCursorPos(0); /* Make a Selection */
    }
    else
        this->_hexeditdata = nullptr;

    this->adjust();
    this->update();

    emit positionChanged(0);
    emit selectionChanged(0);
}

void QHexEditPrivate::setReadOnly(bool b)
{
    this->_readonly = b;
}

void QHexEditPrivate::setAddressWidth(int w)
{
    this->_addressWidth = w;
    this->update();
}

void QHexEditPrivate::setWheelScrollLines(int c)
{
    this->_whellscrolllines = c;
}

void QHexEditPrivate::setCursorPos(qint64 pos)
{
    this->setCursorPos(pos, 0);
}

void QHexEditPrivate::ensureVisible()
{
    qint64 vislines = this->height() / this->_charheight;
    qint64 currline = this->verticalSliderPosition64() + (this->_cursorY / this->_charheight);

    if((currline + 1) < this->verticalSliderPosition64() || (currline + 1) > (this->verticalSliderPosition64() + vislines))
        this->_vscrollbar->setValue((currline + 1) - vislines);
}

void QHexEditPrivate::adjust()
{
    QFontMetrics fm = this->fontMetrics();

    this->_charwidth = fm.width(" ");
    this->_charheight = fm.height();

    this->_xposhex =  this->_charwidth * (this->_addressWidth + 1);
    this->_xposascii = this->_xposhex + (this->_charwidth * (QHexEditPrivate::BYTES_PER_LINE * 3));
    this->_xPosend = this->_xposascii + (this->_charwidth * QHexEditPrivate::BYTES_PER_LINE);

    if(this->_hexeditdata)
    {
        /* Setup ScrollBars */
        qint64 totLines = this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE;
        qint64 visLines = this->height() / this->_charheight;

        /* Setup Vertical ScrollBar */
        if(totLines > visLines)
        {
            this->_vscrollbar->setRange(0, (totLines - visLines) + 1);
            this->_vscrollbar->setSingleStep(1);
            this->_vscrollbar->setPageStep(visLines);
            this->_vscrollbar->show();
        }
        else
            this->_vscrollbar->hide();

        /* Setup Horizontal ScrollBar */
        this->setMinimumWidth(this->_xPosend);
    }
    else
    {
        /* No File Loaded: Hide Scroll Bars */
        this->_vscrollbar->hide();
        this->setMinimumWidth(0);
    }
}

void QHexEditPrivate::setSelectionEnd(qint64 pos, int charidx)
{
    qint64 oldselend = this->selectionEnd();

    if(pos >= this->_hexeditdata->length())
        pos = this->_hexeditdata->length();

    this->_selectionend = pos;
    this->_cursorpos = pos;
    this->_charidx = charidx;

    this->updateCursorXY(this->selectionEnd(), charidx);
    this->update();

    if(oldselend != pos)
        emit selectionChanged(qMax(this->selectionStart(), pos) - qMin(this->selectionStart(), pos));
}

void QHexEditPrivate::setSelection(qint64 start, qint64 end)
{
    if(end == -1)
        end = this->_hexeditdata->length();

    this->_selectionstart = start;
    this->setSelectionEnd(end, 0);

    emit positionChanged(start);
}

void QHexEditPrivate::setRangeColor(qint64 start, qint64 end, QColor color)
{
    for(qint64 i = start; i <= end; i++)
        this->_highlightmap[i] = color;

    this->update();
}

void QHexEditPrivate::removeRangeColor(qint64 start, qint64 end)
{
    for(qint64 i = start; i <= end; i++)
    {
        if(this->_highlightmap.contains(i))
            this->_highlightmap.remove(i);
    }

    this->update();
}

void QHexEditPrivate::resetRangeColor()
{
    if(!this->_highlightmap.isEmpty())
    {
        this->_highlightmap.clear();
        this->update();
    }
}

void QHexEditPrivate::setFont(const QFont &font)
{
    QWidget::setFont(font);
    this->adjust();
}

void QHexEditPrivate::setLineColor(const QColor &c)
{
    this->_selLineColor = c;
    this->update();
}

void QHexEditPrivate::setSelectedCursorBrush(const QBrush &b)
{
    this->_selcursorbrush = b;
    this->update();
}

void QHexEditPrivate::setVerticalScrollBarValue(int value)
{
    this->_vscrollbar->setValue(value);
}

qint64 QHexEditPrivate::indexOf(QByteArray &ba, bool start)
{
    if(this->_hexeditdata)
        return this->_hexeditdata->indexOf(ba, start);

    return -1;
}

qint64 QHexEditPrivate::baseAddress()
{
    return this->_baseaddress;
}

bool QHexEditPrivate::readOnly()
{
    return this->_readonly;
}

int QHexEditPrivate::addressWidth()
{
    return this->_addressWidth;
}

int QHexEditPrivate::visibleLinesCount()
{
    return qCeil(this->height() / this->_charheight);
}

int QHexEditPrivate::wheelScrollLines()
{
    return this->_whellscrolllines;
}

qint64 QHexEditPrivate::cursorPos()
{
    return this->_cursorpos;
}

qint64 QHexEditPrivate::selectionStart()
{
    return qMin(this->_selectionstart, this->_selectionend);
}

qint64 QHexEditPrivate::selectionEnd()
{
    return qMax(this->_selectionstart, this->_selectionend);
}

qint64 QHexEditPrivate::visibleStartOffset()
{
    return this->verticalSliderPosition64() * QHexEditPrivate::BYTES_PER_LINE;
}

qint64 QHexEditPrivate::visibleEndOffset()
{
    qint64 endoff = this->visibleStartOffset() + ((this->height() / this->_charheight) * QHexEditPrivate::BYTES_PER_LINE) - 1;
    return qMin(endoff, this->_hexeditdata->length()); /* Adjust to EOF, if needed */
}

QHexEditData *QHexEditPrivate::data()
{
    return this->_hexeditdata;
}

void QHexEditPrivate::setAddressForeColor(const QColor &c)
{
    this->_addressforecolor = c;
    this->update();
}

void QHexEditPrivate::setAddressBackColor(const QColor &c)
{
    this->_addressbackcolor = c;
    this->update();
}

void QHexEditPrivate::setAlternateLineColor(const QColor &c)
{
    this->_alternatelinecolor = c;
    this->update();
}

QColor& QHexEditPrivate::lineColor()
{
    return this->_selLineColor;
}

QBrush& QHexEditPrivate::selectedCursorBrush()
{
    return this->_selcursorbrush;
}

void QHexEditPrivate::internalSetCursorPos(qint64 pos, int charidx)
{
    if(pos > this->_hexeditdata->length()) /* No >= because we can add text at EOS */
    {
        int currLine = this->_selectionstart / QHexEditPrivate::BYTES_PER_LINE;
        int lastLine = this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE;

        if(currLine == lastLine)
            pos = this->_selectionstart;
        else
            pos = this->_hexeditdata->length();
    }

    this->_cursorpos = this->_selectionend = this->_selectionstart = pos;
    this->_charidx = charidx;
}

QColor QHexEditPrivate::byteWeight(uchar b)
{
    QColor c = this->palette().color(QPalette::WindowText);

    if(!b)
        c = QColor(Qt::darkGray);

    return c;
}

bool QHexEditPrivate::isTextSelected()
{
    return this->_selectionstart != this->_selectionend;
}

void QHexEditPrivate::removeSelectedText()
{
    qint64 start = qMin(this->selectionStart(), this->selectionEnd());
    qint64 end = qMax(this->selectionStart(), this->selectionEnd());

    this->_hexeditdata->remove(start, end - start);
}

void QHexEditPrivate::processDeleteEvents()
{
    if(this->isTextSelected())
        this->removeSelectedText();
    else if(this->_insmode == QHexEditPrivate::Overwrite)
    {
        if(this->_selpart == HexPart)
        {
            uchar hexval = this->_hexeditdata->at(this->selectionStart());

            if(this->_charidx == 1) /* Change Byte's Low Part */
                hexval = (hexval & 0xF0);
            else /* Change Byte's High Part */
                hexval = (hexval & 0x0F);

            this->_hexeditdata->replace(this->selectionStart(), hexval);
        }
        else
            this->_hexeditdata->replace(this->selectionStart(), 0x00);
    }
    else
        this->_hexeditdata->remove(this->selectionStart(), 1);
}

void QHexEditPrivate::processBackspaceEvents()
{
    if(this->isTextSelected())
        this->removeSelectedText();
    else if(this->selectionStart())
    {
        if(this->_insmode == QHexEditPrivate::Overwrite)
            this->_hexeditdata->replace(this->selectionStart() - 1, 0x00);
        else
            this->_hexeditdata->remove(this->selectionStart() - 1, 1);
    }
}

void QHexEditPrivate::processHexPart(int key)
{
    if(this->isTextSelected())
        this->removeSelectedText();

    uchar val = static_cast<uchar>(QString(key).toUInt(NULL, 16));

    if((this->_insmode == QHexEditPrivate::Insert) && !this->_charidx) /* Insert a new byte */
    {
        /* Insert Mode: Add one byte in current position */
        uchar hexval = val << 4; /* X0 Byte */
        this->_hexeditdata->insert(this->selectionStart(), hexval);
    }
    else if((this->_hexeditdata->length() > 0) && (this->selectionStart() < this->_hexeditdata->length()))
    {
         /* Override mode, or update low nibble */
        uchar hexval = this->_hexeditdata->at(this->selectionStart());

        if(this->_charidx == 1) /* Change Byte's Low Part */
            hexval = (hexval & 0xF0) + val;
        else /* Change Byte's High Part */
            hexval = (hexval & 0x0F) + (val << 4);

        this->_hexeditdata->replace(this->selectionStart(), hexval);
    }
}

void QHexEditPrivate::processAsciiPart(int key)
{
    if(this->isTextSelected())
        this->removeSelectedText();

    /* Insert Mode: Add one byte in current position */
    if(this->_insmode == QHexEditPrivate::Insert)
        this->_hexeditdata->insert(this->selectionStart(), static_cast<uchar>(key));
    else if(this->_insmode == QHexEditPrivate::Overwrite && ((this->_hexeditdata->length() > 0) && (this->selectionStart() < this->_hexeditdata->length())))
        this->_hexeditdata->replace(this->selectionStart(), (char)key);
}

bool QHexEditPrivate::processMoveEvents(QKeyEvent *event)
{
    bool processed = true;

    if(event->matches(QKeySequence::MoveToNextChar) || event->matches(QKeySequence::MoveToPreviousChar))
    {
        if(this->_selpart == QHexEditPrivate::HexPart)
        {
            if(event->matches(QKeySequence::MoveToNextChar))
            {
                if(this->_charidx == 0)
                    this->setCursorPos(this->selectionStart(), 1);
                else /* if(this->_hexeditbuffer->charIndex() == 1) */
                    this->setCursorPos(this->selectionStart() + 1, 0);
            }
            else if(event->matches(QKeySequence::MoveToPreviousChar))
            {
                if(this->_charidx == 0)
                    this->setCursorPos(this->selectionStart() - 1, 1);
                else /* if(this->_hexeditbuffer->charIndex() == 1) */
                    this->setCursorPos(this->selectionStart(), 0);
            }
        }
        else if(this->_selpart == QHexEditPrivate::AsciiPart)
        {
            if(event->matches(QKeySequence::MoveToNextChar))
                this->setCursorPos(this->selectionStart() + 1, 0);
            else if(event->matches(QKeySequence::MoveToPreviousChar))
                this->setCursorPos(this->selectionStart() - 1, 0);
        }
    }
    else if(event->matches(QKeySequence::MoveToNextLine))
        this->setCursorPos(this->selectionStart() + QHexEditPrivate::BYTES_PER_LINE, this->_charidx);
    else if(event->matches(QKeySequence::MoveToPreviousLine))
        this->setCursorPos(this->selectionStart() - QHexEditPrivate::BYTES_PER_LINE, this->_charidx);
    else if(event->matches(QKeySequence::MoveToStartOfLine))
        this->setCursorPos(this->selectionStart() - (this->selectionStart() % QHexEditPrivate::BYTES_PER_LINE));
    else if(event->matches(QKeySequence::MoveToNextPage))
        this->setCursorPos(this->selectionStart() + (((this->height() / this->_charheight) - 1) * QHexEditPrivate::BYTES_PER_LINE));
    else if(event->matches(QKeySequence::MoveToPreviousPage))
        this->setCursorPos(this->selectionStart() - (((this->height() / this->_charheight) - 1) * QHexEditPrivate::BYTES_PER_LINE));
    else if(event->matches(QKeySequence::MoveToEndOfLine))
        this->setCursorPos(this->selectionStart() | (QHexEditPrivate::BYTES_PER_LINE - 1));
    else if(event->matches(QKeySequence::MoveToStartOfDocument))
        this->setCursorPos(0);
    else if(event->matches(QKeySequence::MoveToEndOfDocument))
        this->setCursorPos(this->_hexeditdata->length());
    else
        processed = false;

    return processed;
}

bool QHexEditPrivate::processSelectEvents(QKeyEvent *event)
{
    bool processed = true;

    if(event->matches(QKeySequence::SelectNextChar) || event->matches(QKeySequence::SelectPreviousChar))
    {
        if(this->_selpart == QHexEditPrivate::HexPart)
        {
            if(event->matches(QKeySequence::SelectNextChar))
            {
                if(this->_charidx == 0)
                    this->setSelectionEnd(this->selectionEnd(), 1);
                else /* if(this->_hexeditbuffer->charIndex() == 1) */
                    this->setSelectionEnd(this->selectionEnd() + 1, 0);
            }
            else if(event->matches(QKeySequence::SelectPreviousChar))
            {
                if(this->_charidx == 0)
                    this->setSelectionEnd(this->selectionEnd() - 1, 1);
                else /* if(this->_hexeditbuffer->charIndex() == 1) */
                    this->setSelectionEnd(this->selectionEnd(), 0);
            }
        }
        else if(this->_selpart == QHexEditPrivate::AsciiPart)
        {
            if(event->matches(QKeySequence::SelectNextChar))
                this->setSelectionEnd(this->selectionEnd() + 1, 0);
            else if(event->matches(QKeySequence::SelectPreviousChar))
                this->setSelectionEnd(this->selectionEnd() - 1, 0);
        }
    }
    else if(event->matches(QKeySequence::SelectNextLine))
        this->setSelectionEnd(this->selectionEnd() + QHexEditPrivate::BYTES_PER_LINE, this->_charidx);
    else if(event->matches(QKeySequence::SelectPreviousLine))
        this->setSelectionEnd(this->selectionEnd() - QHexEditPrivate::BYTES_PER_LINE, this->_charidx);
    else if(event->matches(QKeySequence::SelectStartOfDocument))
        this->setSelection(this->selectionEnd(), 0);
    else if(event->matches(QKeySequence::SelectEndOfDocument))
        this->setSelectionEnd(this->_hexeditdata->length(), 0);
    else if(event->matches(QKeySequence::SelectAll))
        this->setSelection(0, this->_hexeditdata->length());
    else
        processed = false;

    return processed;
}

bool QHexEditPrivate::processTextInputEvents(QKeyEvent *event)
{
    if(event->modifiers() & Qt::ControlModifier)
        return false;

    bool processed = true;
    int key = static_cast<int>(event->text()[0].toLatin1());

    if((this->_selpart == HexPart) && ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f'))) /* Check if is a Hex Char */
        this->processHexPart(key);
    else if((this->_selpart == QHexEditPrivate::AsciiPart) && (key >= 0x20 && key <= 0x7E)) /* Check if is a Printable Char */
        this->processAsciiPart(key);
    else if(event->key() == Qt::Key_Delete)
        this->processDeleteEvents();
    else if((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
        this->processBackspaceEvents();
    else
        processed = false;

    return processed;
}

bool QHexEditPrivate::processInsOvrEvents(QKeyEvent *event)
{
    if((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        this->_insmode = (this->_insmode == QHexEditPrivate::Overwrite ? QHexEditPrivate::Insert : QHexEditPrivate::Overwrite);
        return true;
    }

    return false;
}

bool QHexEditPrivate::processUndoRedo(QKeyEvent *event)
{
    bool processed = true;

    if(event->matches(QKeySequence::Undo))
        this->undo();
    else if(event->matches(QKeySequence::Redo))
        this->redo();
    else
        processed = false;

    return processed;
}

bool QHexEditPrivate::processClipboardKeys(QKeyEvent *event)
{
    bool processed = true;

    if(event->matches(QKeySequence::Cut))
        this->cut();
    else if(event->matches(QKeySequence::Copy))
        this->copy();
    else if(event->matches(QKeySequence::Paste))
        this->paste();
    else
        processed = false;

    return processed;
}

void QHexEditPrivate::setCursorPos(qint64 pos, int charidx)
{
    if(pos < 0)
        pos = 0;
    else if(pos > this->_hexeditdata->length())
        pos = this->_hexeditdata->length();

    qint64 oldpos = this->cursorPos();
    int oldcharidx = this->_charidx;
    qint64 oldsellength = this->selectionEnd() - this->selectionStart();

    /* Delete Cursor */
    this->_blink = false;
    this->update();

    this->internalSetCursorPos(pos, charidx);
    this->updateCursorXY(this->cursorPos(), charidx);

    /* Redraw Cursor NOW! */
    this->_blink = true;
    this->update();

    this->ensureVisible();

    if(oldsellength != (this->selectionEnd() - this->selectionStart()))
        emit selectionChanged(0);

    if(oldpos != pos || oldcharidx != charidx)
        emit positionChanged(pos);
}

void QHexEditPrivate::updateCursorXY(qint64 pos, int charidx)
{
    this->_cursorY = ((pos - (this->verticalSliderPosition64() * QHexEditPrivate::BYTES_PER_LINE)) / QHexEditPrivate::BYTES_PER_LINE) * this->_charheight;

    if(this->_selpart == AddressPart || this->_selpart == HexPart)
    {
        qint64 x = pos % QHexEditPrivate::BYTES_PER_LINE;
        this->_cursorX = x * (3 * this->_charwidth) + this->_xposhex;

        if(charidx)
            this->_cursorX += this->_charwidth;
    }
    else /* if this->_selPart == AsciiPart */
    {
        qint64 x = pos % QHexEditPrivate::BYTES_PER_LINE;
        this->_cursorX = x * this->_charwidth + this->_xposascii;
    }
}

void QHexEditPrivate::drawParts(QPainter &painter)
{
    int span = this->_charwidth / 2;
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.setPen(this->palette().color(QPalette::WindowText));
    painter.drawLine(this->_xposhex - span, 0, this->_xposhex - span, this->height());
    painter.drawLine(this->_xposascii - span, 0, this->_xposascii - span, this->height());
    painter.drawLine(this->_xPosend + span, 0, this->_xPosend + span, this->height());
}

QColor& QHexEditPrivate::addressForeColor()
{
    return this->_addressforecolor;
}

QColor& QHexEditPrivate::addressBackColor()
{
    return this->_addressbackcolor;
}

QColor &QHexEditPrivate::alternateLineColor()
{
    return this->_alternatelinecolor;
}

void QHexEditPrivate::drawLine(QPainter &painter, QFontMetrics &fm, qint64 line, int y)
{
    this->drawAddress(painter, fm, line, y);
    this->drawHexPart(painter, fm, line, y);
    this->drawAsciiPart(painter, fm, line, y);
}

void QHexEditPrivate::drawAddress(QPainter &painter, QFontMetrics &fm, qint64 line, int y)
{
    int lineMax = this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE;
    painter.fillRect(0, y, this->_xposhex - (this->_charwidth / 2), this->_charheight, this->_addressbackcolor);

    if(line <= lineMax)
    {
        QString addr = QString("%1").arg(this->_baseaddress + (line * QHexEditPrivate::BYTES_PER_LINE), this->_addressWidth, 16, QLatin1Char('0')).toUpper();

        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(this->_addressforecolor);
        painter.drawText(0, y, fm.width(addr), this->_charheight, Qt::AlignLeft | Qt::AlignTop, addr);
    }
}

void QHexEditPrivate::drawHexPart(QPainter &painter, QFontMetrics& fm, qint64 line, int y)
{
    int x = this->_xposhex;
    qint64 linelength = QHexEditPrivate::BYTES_PER_LINE;

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(this->palette().base());

    qint64 linestart = line * QHexEditPrivate::BYTES_PER_LINE;
    qint64 lineEnd = linestart + QHexEditPrivate::BYTES_PER_LINE;

    if(this->cursorPos() >= linestart && this->cursorPos() < lineEnd) /* This is the Selected Line! */
        painter.fillRect(this->_xposhex - (this->_charwidth / 2), y, this->_xposascii - this->_xposhex, this->_charheight, this->_selLineColor);
    else if(line & 1)
        painter.fillRect(this->_xposhex - (this->_charwidth / 2), y, this->_xposascii - this->_xposhex, this->_charheight, this->_alternatelinecolor);

    for(qint64 i = 0; i < linelength; i++)
    {
        bool highlighted = false;
        qint64 pos = (line * linelength) + i;

        if(pos >= this->_hexeditdata->length())
            return; /* Reached EOF */

        uchar b = this->_hexeditdata->at(linestart + i);
        QString s = QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
        int w = fm.width(s);

        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(this->byteWeight(b));

        if(pos == this->selectionStart() && (this->_selpart == AsciiPart || !this->hasFocus()))
        {
            highlighted = true;
            w += this->_charwidth;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(this->_selcursorbrush);
        }

        if(this->selectionStart() != this->selectionEnd()) /* Highlight Selection */
        {
            qint64 start = qMin(this->selectionStart(), this->selectionEnd());
            qint64 end = qMax(this->selectionStart(), this->selectionEnd());

            if((pos >= start && pos < end))
            {
                if(i < (linelength - 1))
                {
                    s.append(" "); /* Append a WhiteSpace */
                    w = fm.width(s);
                }

                highlighted = true;
                painter.setBackgroundMode(Qt::OpaqueMode);
                painter.setBackground(this->palette().brush(QPalette::Highlight));
                painter.setPen(this->palette().color(QPalette::HighlightedText));
            }
        }

        if(!highlighted && this->_highlightmap.contains(pos)) /* Highlight Range */
        {
            if(i < (linelength - 1))
            {
                s.append(" "); /* Append a WhiteSpace */
                w = fm.width(s);
            }

            highlighted = true;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(QBrush(this->_highlightmap[pos]));
            painter.setPen(this->palette().color(QPalette::WindowText));
        }

        painter.drawText(x, y, w, this->_charheight, Qt::AlignLeft | Qt::AlignTop, s);

        if(!highlighted)
            w += this->_charwidth; /* Character not selected, add a whitespace (pixel span, not true space) */

        x += w;
    }
}

void QHexEditPrivate::drawAsciiPart(QPainter &painter, QFontMetrics& fm, qint64 line, int y)
{
    int x = this->_xposascii;

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(this->palette().base());
    painter.setPen(this->palette().color(QPalette::WindowText));

    int span = this->_charwidth / 2;
    qint64 linestart = line * QHexEditPrivate::BYTES_PER_LINE;
    qint64 lineend = linestart + QHexEditPrivate::BYTES_PER_LINE;

    if(this->cursorPos() >= linestart && this->cursorPos() < lineend) /* This is the Selected Line! */
        painter.fillRect(this->_xposascii - span, y, (this->_xPosend - this->_xposascii) + this->_charwidth, this->_charheight, this->_selLineColor);
    else if(line & 1)
        painter.fillRect(this->_xposascii - span, y, (this->_xPosend - this->_xposascii) + this->_charwidth, this->_charheight, this->_alternatelinecolor);

    for(qint64 i = 0; i < QHexEditPrivate::BYTES_PER_LINE; i++)
    {
        bool highlighted = false;
        qint64 pos = (line * QHexEditPrivate::BYTES_PER_LINE) + i;

        if(pos >= this->_hexeditdata->length())
            return; /* Reached EOF */

        if(pos == this->selectionStart() && (this->_selpart == QHexEditPrivate::HexPart || !this->hasFocus()))
        {
            highlighted = true;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(this->_selcursorbrush);
        }

        if(this->selectionStart() != this->selectionEnd()) /* Highlight Selection */
        {
            qint64 start = qMin(this->selectionStart(), this->selectionEnd());
            qint64 end = qMax(this->selectionStart(), this->selectionEnd());

            if(pos >= start && pos < end)
            {
                highlighted = true;
                painter.setBackgroundMode(Qt::OpaqueMode);
                painter.setBackground(this->palette().brush(QPalette::Highlight));
                painter.setPen(this->palette().color(QPalette::HighlightedText));
            }
        }

        if(!highlighted && this->_highlightmap.contains(pos)) /* Highlight Range */
        {
            highlighted = true;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(QBrush(this->_highlightmap[pos]));
            painter.setPen(this->palette().color(QPalette::WindowText));
        }

        if(!highlighted)
        {
            painter.setBackgroundMode(Qt::TransparentMode);
            painter.setPen(this->palette().color(QPalette::WindowText));
        }

        int w = 0;
        QChar b = this->_hexeditdata->at(linestart + i);

        if(b.isPrint())
        {
            w = fm.width(b);
            painter.drawText(x, y, w, this->_charheight, Qt::AlignLeft | Qt::AlignTop, QString(b));
        }
        else
        {
            w = fm.width(".");

            if(!highlighted)
                painter.setPen(QColor(Qt::darkGray)); /* Make Non-Ascii bytes less visible */

            painter.drawText(x, y, w, this->_charheight, Qt::AlignLeft | Qt::AlignTop, ".");
        }

        x += w;
    }
}

qint64 QHexEditPrivate::cursorPosFromPoint(const QPoint &pt, int *charindex)
{
    qint64 y = (this->verticalSliderPosition64() + (pt.y() / this->_charheight)) * QHexEditPrivate::BYTES_PER_LINE, x = 0;
    *charindex = 0;

    if(this->_selpart == HexPart)
    {
        x = (pt.x() - this->_xposhex) / this->_charwidth;

        if((x % 3) != 0) /* Is Second Nibble Selected? */
            *charindex = 1;

        x = x / 3;
    }
    else if(this->_selpart == AsciiPart)
        x = ((pt.x() - this->_xposascii) / this->_charwidth);
    /* else
        is the address part: x = 0 */

    return y + x;
}

qint64 QHexEditPrivate::verticalSliderPosition64()
{
    return static_cast<qint64>(this->_vscrollbar->sliderPosition());
}

void QHexEditPrivate::paintEvent(QPaintEvent* pe)
{
    QPainter painter(this);

    if(this->_hexeditdata)
    {
        QRect r = pe->rect();
        QFontMetrics fm = this->fontMetrics();
        qint64 slidepos = this->_vscrollbar->isVisible() ? this->verticalSliderPosition64() : 0;
        qint64 start = slidepos + (r.top() / this->_charheight), end = (slidepos + (r.bottom() / this->_charheight)) + 1; /* end + 1 Removes the scroll bug */

        for(qint64 i = start; i <= end; i++)
        {
            int y = (i - slidepos) * this->_charheight;
            this->drawLine(painter, fm, i, y);
        }
    }

    this->drawParts(painter);

    if(this->_hexeditdata && this->hasFocus() && this->_blink)
    {
        if(this->_insmode == QHexEditPrivate::Overwrite)
            painter.fillRect(this->_cursorX, this->_cursorY + this->_charheight - 3, this->_charwidth, 2, this->palette().color(QPalette::WindowText));
        else
            painter.fillRect(this->_cursorX, this->_cursorY, 2, this->_charheight, this->palette().color(QPalette::WindowText));
    }
}

void QHexEditPrivate::mousePressEvent(QMouseEvent* event)
{
    if(this->_hexeditdata)
    {
        if(event->buttons() & Qt::LeftButton)
        {
            QPoint pos = event->pos();

            if(pos.x() && pos.y())
            {
                this->_blink = false;
                this->update();

                int span = this->_charheight / 2;

                if(pos.x() < (this->_xposhex - span))
                    this->_selpart = AddressPart;
                else if(pos.x() >= (this->_xposascii - span))
                    this->_selpart = AsciiPart;
                else
                    this->_selpart = HexPart;

                int charidx = 0;
                qint64 cPos = this->cursorPosFromPoint(pos, &charidx);

                if(event->modifiers() == Qt::ShiftModifier)
                    this->setSelectionEnd(cPos, charidx);
                else if(event->modifiers() == Qt::NoModifier)
                    this->setCursorPos(cPos, charidx);
            }
        }
    }
}

void QHexEditPrivate::mouseMoveEvent(QMouseEvent* event)
{
    if(this->_hexeditdata)
    {
        if(event->buttons() & Qt::LeftButton)
        {
            QPoint pos = event->pos();

            if(pos.x() && pos.y())
            {
                this->_blink = false;
                this->update();

                int charidx = 0;
                qint64 cPos = this->cursorPosFromPoint(pos, &charidx);
                this->setSelectionEnd(cPos, charidx);
            }
        }
    }
}

void QHexEditPrivate::wheelEvent(QWheelEvent *event)
{
    if(this->_hexeditdata->length())
    {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;

        if(event->orientation() == Qt::Vertical)
        {
            int pos = this->verticalSliderPosition64() - (numSteps * this->_whellscrolllines);
            int maxlines = this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE;

            /* Bounds Check */
            if(pos < 0)
                pos = 0;
            else if(pos > maxlines)
                pos = maxlines;

            this->_vscrollbar->setSliderPosition(pos);
            this->updateCursorXY(this->cursorPos(), this->_charidx);
            this->update();

            event->accept();
        }
    }
    else
        event->ignore();
}

void QHexEditPrivate::keyPressEvent(QKeyEvent* event)
{
    if(!this->_hexeditdata)
        return;

    this->_lastkeyevent = event; /* Save Event */
    bool processed = this->processMoveEvents(event);

    if(!processed)
        processed = this->processSelectEvents(event);

    if(!processed)
        processed = this->processInsOvrEvents(event);

    if(!processed)
        processed = this->processUndoRedo(event);

    if(!processed)
        processed = this->processClipboardKeys(event);

    if(!this->_readonly && !processed)
        this->processTextInputEvents(event);
}

void QHexEditPrivate::resizeEvent(QResizeEvent*)
{
    this->adjust(); /* Update ScrollBars */
}

void QHexEditPrivate::blinkCursor()
{
    this->_blink = !this->_blink;
    this->update(this->_cursorX, this->_cursorY, this->_charwidth, this->_charheight);
}

void QHexEditPrivate::vScrollBarValueChanged(int)
{
    this->update();
}

void QHexEditPrivate::hexEditDataChanged(qint64 offset, qint64, QHexEditData::ActionType reason)
{
    if(this->_lastkeyevent)
    {
        if(this->_lastkeyevent->key() == Qt::Key_Delete)
        {
            this->setSelectionEnd(this->selectionStart(), 0); /* Update ScrollBars only, don't move the cursor, reset selection */
            this->adjust();
            this->ensureVisible();
            this->update();
            return;
        }

        if(this->_lastkeyevent->key() == Qt::Key_Backspace)
            this->setCursorPos(offset);
        else if(this->_selpart == QHexEditPrivate::AsciiPart)
        {
            if(this->_lastkeyevent->matches(QKeySequence::Undo))
                this->setCursorPos(offset);
            else if(reason == QHexEditData::Insert || reason == QHexEditData::Replace)
                this->setCursorPos(offset + 1);
        }
        else if(this->_selpart == QHexEditPrivate::HexPart)
        {
            if(reason == QHexEditData::Insert || reason == QHexEditData::Replace)
            {
                if(this->_charidx == 1)
                    this->setCursorPos(offset + 1, 0);
                else
                    this->setCursorPos(offset, 1);
            }
        }
    }

    this->adjust(); /* Update ScrollBars */
    this->ensureVisible();
    this->update();
}
