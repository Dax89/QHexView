#include "qhexeditprivate.h"

QString QHexEditPrivate::UNPRINTABLE_CHAR;
const int QHexEditPrivate::CURSOR_BLINK_INTERVAL = 500; /* 0.5 sec */
const qint64 QHexEditPrivate::BYTES_PER_LINE = 0x10;

QHexEditPrivate::QHexEditPrivate(QScrollArea *scrollarea, QScrollBar *vscrollbar, QWidget *parent): QWidget(parent)
{
    if(QHexEditPrivate::UNPRINTABLE_CHAR.isEmpty())
        QHexEditPrivate::UNPRINTABLE_CHAR = ".";

    this->_lastkeyevent = nullptr;
    this->_writer = nullptr;
    this->_reader = nullptr;
    this->_highlighter = nullptr;
    this->_comments = nullptr;
    this->_hexeditdata = nullptr;
    this->_scrollarea = scrollarea;
    this->_vscrollbar = vscrollbar;
    this->_blink = this->_readonly = false;
    this->_lastvisiblelines = this->_lastvscrollpos = this->_baseaddress = this->_cursorX = this->_cursorY = this->_cursorpos = this->_selectionstart = this->_selectionend = this->_charidx = this->_charheight = this->_charwidth = 0;
    this->_selpart = QHexEditPrivate::HexPart;
    this->_insmode = QHexEditPrivate::Overwrite;

    connect(this->_vscrollbar, SIGNAL(valueChanged(int)), this, SLOT(onVScrollBarValueChanged(int)));
    connect(this->_vscrollbar, SIGNAL(valueChanged(int)), this, SIGNAL(verticalScrollBarValueChanged(int)));

    QFont f("Monospace", qApp->font().pointSize());
    f.setStyleHint(QFont::TypeWriter); /* Use monospace fonts! */

    this->setMouseTracking(true);
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
        QByteArray ba = this->_reader->read(start, end - start);

        cpbd->setText(QString(ba));
        this->_writer->remove(start, end - start);
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
        QByteArray ba = this->_reader->read(start, end - start);
        cpbd->setText(QString::fromLatin1(ba.constData(), ba.length()));
    }
}

void QHexEditPrivate::copyHex()
{
    qint64 start = qMin(this->_selectionstart, this->_selectionend);
    qint64 end = qMax(this->_selectionstart, this->_selectionend);

    if(end - start)
    {
        QClipboard* cpbd = qApp->clipboard();
        QByteArray ba = this->_reader->read(start, end - start);
        cpbd->setText(QString(ba.toHex()).toUpper());
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
        qint64 len = end - start;

        if(len)
            this->_writer->replace(start, len, ba);
        else if(this->_insmode == QHexEditPrivate::Overwrite)
            this->_writer->replace(start, ba.length(), ba);
        else
            this->_writer->insert(start, ba);

        this->setCursorPos((start + s.length()), 0);
        this->adjust();
        this->ensureVisible();
    }
}

void QHexEditPrivate::pasteHex()
{
    QClipboard* cpbd = qApp->clipboard();
    QString s = cpbd->text();

    if(!s.isNull())
    {
        QByteArray ba = QByteArray::fromHex(s.toLatin1());
        qint64 start = qMin(this->_selectionstart, this->_selectionend);
        qint64 end = qMax(this->_selectionstart, this->_selectionend);
        qint64 len = end - start;

        if(len)
            this->_writer->replace(start, len, ba);
        else if(this->_insmode == QHexEditPrivate::Overwrite)
            this->_writer->replace(start, ba.length(), ba);
        else
            this->_writer->insert(start, ba);

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
        this->_reader = new QHexEditDataReader(hexeditdata, hexeditdata);
        this->_writer = new QHexEditDataWriter(hexeditdata, hexeditdata);
        this->_highlighter = new QHexEditHighlighter(hexeditdata, QColor(Qt::transparent), this->palette().color(QPalette::WindowText), this);
        this->_comments = new QHexEditComments(this);

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

    if(currline <= this->verticalSliderPosition64() || currline >= (this->verticalSliderPosition64() + vislines))
        this->_vscrollbar->setValue(qMax(currline - (vislines / 2), qint64(0)));
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
    qint64 oldselendlength = this->selectionEnd() - this->selectionStart();

    if(pos >= this->_hexeditdata->length())
        pos = this->_hexeditdata->length();

    this->_selectionend = pos;
    this->_charidx = charidx;

    this->updateCursorXY(pos, charidx);
    this->update();

    qint64 len = this->selectionEnd() - this->selectionStart();

    if(oldselendlength != len)
        emit selectionChanged(qMax(this->selectionStart(), pos) - qMin(this->selectionStart(), pos));
}

void QHexEditPrivate::setSelection(qint64 start, qint64 end)
{
    if(end == -1)
        end = this->_hexeditdata->length();

    this->setCursorPos(start); /* Updates selectionstart too */
    this->setSelectionEnd(end + 1, 0); /* 'end' is inclusive */

    this->ensureVisible();

    emit positionChanged(start);
}

void QHexEditPrivate::highlightForeground(qint64 start, qint64 end, const QColor &color)
{
    if(!this->_highlighter)
        return;

    this->_highlighter->highlightForeground(start, end, color);
    this->update();
}

void QHexEditPrivate::highlightBackground(qint64 start, qint64 end, const QColor &color)
{
    if(!this->_highlighter)
        return;

    this->_highlighter->highlightBackground(start, end, color);
    this->update();
}

void QHexEditPrivate::clearHighlight(qint64 start, qint64 end)
{
    if(!this->_highlighter)
        return;

    this->_highlighter->clearHighlight(start, end);
    this->update();
}

void QHexEditPrivate::clearHighlight()
{
    if(this->_hexeditdata)
        this->clearHighlight(0, this->_hexeditdata->length());
}

void QHexEditPrivate::commentRange(qint64 start, qint64 end, const QString &comment)
{
    if(!this->_comments)
        return;

    this->_comments->commentRange(start, end, comment);
}

void QHexEditPrivate::uncommentRange(qint64 start, qint64 end)
{
    if(!this->_comments)
        return;

    this->_comments->uncommentRange(start, end);
}

void QHexEditPrivate::clearComments()
{
    if(!this->_comments)
        return;

    this->_comments->clearComments();
}

void QHexEditPrivate::setFont(const QFont &font)
{
    QWidget::setFont(font);
    this->adjust();
}

void QHexEditPrivate::setLineColor(const QColor &c)
{
    this->_sellinecolor = c;
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

void QHexEditPrivate::scroll(QWheelEvent *event)
{
    this->wheelEvent(event);
}

qint64 QHexEditPrivate::indexOf(QByteArray &ba, qint64 start)
{
    if(this->_hexeditdata)
        return this->_reader->indexOf(ba, start);

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
    return qMin(this->verticalSliderPosition64() * QHexEditPrivate::BYTES_PER_LINE, this->_hexeditdata->length()); /* Adjust to EOF, if needed */
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
    return this->_sellinecolor;
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

void QHexEditPrivate::checkVisibleLines()
{
    qint64 vislines = this->visibleLinesCount();
    qint64 vscrollpos = this->verticalSliderPosition64();

    if((vislines != this->_lastvisiblelines) || (vscrollpos != this->_lastvscrollpos))
    {
        this->_lastvisiblelines = vislines;
        this->_lastvscrollpos = vscrollpos;
        emit visibleLinesChanged();
    }
}

bool QHexEditPrivate::isTextSelected()
{
    return this->_selectionstart != this->_selectionend;
}

void QHexEditPrivate::removeSelectedText()
{
    qint64 start = qMin(this->selectionStart(), this->selectionEnd());
    qint64 end = qMax(this->selectionStart(), this->selectionEnd());

    this->_writer->remove(start, end - start);
}

void QHexEditPrivate::processDeleteEvents()
{
    if(this->isTextSelected())
        this->removeSelectedText();
    else if(this->_insmode == QHexEditPrivate::Overwrite)
    {
        if(this->_selpart == HexPart)
        {
            uchar hexval = this->_reader->at(this->selectionStart());

            if(this->_charidx == 1) /* Change Byte's Low Part */
                hexval = (hexval & 0xF0);
            else /* Change Byte's High Part */
                hexval = (hexval & 0x0F);

            this->_writer->replace(this->selectionStart(), hexval);
        }
        else
            this->_writer->replace(this->selectionStart(), 0x00);
    }
    else
        this->_writer->remove(this->selectionStart(), 1);
}

void QHexEditPrivate::processBackspaceEvents()
{
    if(this->isTextSelected())
        this->removeSelectedText();
    else if(this->selectionStart())
    {
        if(this->_insmode == QHexEditPrivate::Overwrite)
            this->_writer->replace(this->selectionStart() - 1, 0x00);
        else
            this->_writer->remove(this->selectionStart() - 1, 1);
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
        this->_writer->insert(this->selectionStart(), hexval);
    }
    else if((this->_hexeditdata->length() > 0) && (this->selectionStart() < this->_hexeditdata->length()))
    {
         /* Override mode, or update low nibble */
        uchar hexval = this->_reader->at(this->selectionStart());

        if(this->_charidx == 1) /* Change Byte's Low Part */
            hexval = (hexval & 0xF0) + val;
        else /* Change Byte's High Part */
            hexval = (hexval & 0x0F) + (val << 4);

        this->_writer->replace(this->selectionStart(), hexval);
    }
}

void QHexEditPrivate::processAsciiPart(int key)
{
    if(this->isTextSelected())
        this->removeSelectedText();

    /* Insert Mode: Add one byte in current position */
    if(this->_insmode == QHexEditPrivate::Insert)
        this->_writer->insert(this->selectionStart(), static_cast<uchar>(key));
    else if(this->_insmode == QHexEditPrivate::Overwrite && ((this->_hexeditdata->length() > 0) && (this->selectionStart() < this->_hexeditdata->length())))
        this->_writer->replace(this->selectionStart(), (char)key);
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
        this->_blink = true;
        this->update();
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

    if(this->_selpart == QHexEditPrivate::AddressPart || this->_selpart == QHexEditPrivate::HexPart)
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

void QHexEditPrivate::drawLineBackground(QPainter &painter, qint64 line, qint64 linestart, int y)
{
    QRect hexr(this->_xposhex, y, this->_xposascii - (this->_charwidth / 2), this->_charheight);
    QRect asciir(this->_xposascii, y, (this->_charwidth * QHexEditPrivate::BYTES_PER_LINE) + (this->_charwidth / 2), this->_charheight);

    if((this->_cursorpos >= linestart) && (this->_cursorpos < (linestart + QHexEditPrivate::BYTES_PER_LINE))) /* This is the Selected Line */
    {
        painter.fillRect(hexr, this->_sellinecolor);
        painter.fillRect(asciir, this->_sellinecolor);
    }
    else if(line & 1)
    {
        painter.fillRect(hexr, this->_alternatelinecolor);
        painter.fillRect(asciir, this->_alternatelinecolor);
    }
    else
    {
        painter.fillRect(hexr, this->palette().color(QPalette::Base));
        painter.fillRect(asciir, this->palette().color(QPalette::Base));
    }
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
    int xhex = this->_xposhex, xascii = this->_xposascii;
    qint64 linestart = line * QHexEditPrivate::BYTES_PER_LINE;

    painter.setBackgroundMode(Qt::TransparentMode);
    this->drawLineBackground(painter, line, linestart, y);
    this->drawAddress(painter, fm, line, linestart, y);

    for(qint64 i = 0; i < QHexEditPrivate::BYTES_PER_LINE; i++)
    {
        qint64 pos = linestart + i;

        if(pos >= this->_hexeditdata->length())
            return; /* Reached EOF */

        uchar b = this->_reader->at(pos);

        if(this->_comments->isCommented(pos))
        {
            QFont f = this->font();
            f.setBold(true);
            painter.setFont(f);
        }
        else
            painter.setFont(this->font());

        QColor bchex, fchex, bcascii, fcascii;
        this->colorize(b, pos, bchex, fchex, bcascii, fcascii);

        this->drawHex(painter, fm, bchex, fchex, b, i, xhex, y);
        this->drawAscii(painter, fm, bcascii, fcascii, b, xascii, y);
    }
}

void QHexEditPrivate::drawAddress(QPainter &painter, QFontMetrics &fm, qint64 line, qint64 linestart, int y)
{
    qint64 linemax = this->_hexeditdata->length() / QHexEditPrivate::BYTES_PER_LINE;
    painter.fillRect(0, y, this->_xposhex - (this->_charwidth / 2), this->_charheight, this->_addressbackcolor);

    if((this->_hexeditdata && line == 0) || (line < linemax))
    {
        QString addr = QString("%1").arg(this->_baseaddress + (line * QHexEditPrivate::BYTES_PER_LINE), this->_addressWidth, 16, QLatin1Char('0')).toUpper();

        if((this->_cursorpos >= linestart) && (this->_cursorpos < (linestart + QHexEditPrivate::BYTES_PER_LINE))) /* This is the Selected Line */
            painter.setPen(Qt::red);
        else
            painter.setPen(this->_addressforecolor);

        painter.drawText(0, y, fm.width(addr), this->_charheight, Qt::AlignLeft | Qt::AlignTop, addr);
    }
}

void QHexEditPrivate::drawHex(QPainter &painter, QFontMetrics &fm, const QColor& bc, const QColor &fc, uchar b, qint64 i, int &x, int y)
{
    QString s = QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
    int w = fm.width(s);
    QRect r(x, y, w, this->_charheight);

    if(i < (QHexEditPrivate::BYTES_PER_LINE - 1))
        r.setWidth(r.width() + this->_charwidth);

    painter.fillRect(r, bc);
    painter.setPen(fc);
    painter.drawText(x, y, w, this->_charheight, Qt::AlignLeft | Qt::AlignTop, s);

    x += r.width();
}

void QHexEditPrivate::drawAscii(QPainter &painter, QFontMetrics &fm, const QColor &bc, const QColor &fc, uchar b, int &x, int y)
{
    int w;
    QString s;

    if(QChar(b).isPrint())
    {
        w = fm.width(b);
        s = QString(b);
    }
    else
    {
        w = fm.width(QHexEditPrivate::UNPRINTABLE_CHAR);
        s = QHexEditPrivate::UNPRINTABLE_CHAR;
    }

    QRect r(x, y, w, this->_charheight);
    painter.fillRect(r, bc);
    painter.setPen(fc);
    painter.drawText(r, Qt::AlignLeft | Qt::AlignTop, s);
    x += w;
}

qint64 QHexEditPrivate::cursorPosFromPoint(const QPoint &pt, int *charindex)
{
    qint64 y = (this->verticalSliderPosition64() + (pt.y() / this->_charheight)) * QHexEditPrivate::BYTES_PER_LINE, x = 0;

    if(charindex)
        *charindex = 0;

    if(this->_selpart == HexPart)
    {
        x = (pt.x() - this->_xposhex) / this->_charwidth;

        if(charindex && (x % 3) != 0) /* Is Second Nibble Selected? */
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

void QHexEditPrivate::colorize(uchar b, qint64 pos, QColor &bchex, QColor &fchex, QColor &bcascii, QColor &fcascii)
{
    if((this->_selectionstart != this->_selectionend) && (pos >= this->selectionStart()) && (pos < this->selectionEnd())) /* Selected Text */
    {
        bchex = bcascii = this->palette().color(QPalette::Highlight);
        fchex = fcascii = this->palette().color(QPalette::HighlightedText);
    }
    else
    {
        QColor bc, fc;
        this->_highlighter->colors(pos, bc, fc);

        if((this->_selpart == QHexEditPrivate::AsciiPart || this->_selpart == QHexEditPrivate::HexPart) && (this->_selectionstart == this->_selectionend) && (pos == this->_cursorpos))
        {
            if(this->_selpart == QHexEditPrivate::AsciiPart)
            {
                bchex = QColor(Qt::lightGray);
                bcascii = bc;
            }
            else /* if(this->_selpart == QHexEditPrivate::HexPart) */
            {
                bchex = bc;
                bcascii = QColor(Qt::lightGray);
            }
        }
        else
            bchex = bcascii = bc;

        if(b == 0x00 || b == 0xFF)
            fchex = QColor(Qt::darkGray);
        else
            fchex = fc;

        fcascii = (QChar(b).isPrint() ? fc : QColor(Qt::darkGray));
    }
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
        if((event->buttons() == Qt::NoButton) && this->_comments)
        {
            qint64 offset = this->cursorPosFromPoint(event->pos(), nullptr);

            if(this->_comments->isCommented(offset))
                this->_comments->displayNote(this->mapToGlobal(event->pos()), offset);
            else
                this->_comments->hideNote();
        }
        else if(event->buttons() & Qt::LeftButton)
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
    this->checkVisibleLines();
}

void QHexEditPrivate::blinkCursor()
{
    this->_blink = !this->_blink;
    this->update(this->_cursorX, this->_cursorY, this->_charwidth, this->_charheight);
}

void QHexEditPrivate::onVScrollBarValueChanged(int)
{
    this->checkVisibleLines();
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
