#include "qhexeditprivate.h"

const int QHexEditPrivate::CURSOR_BLINK_INTERVAL = 500; /* 0.5 sec */

QHexEditPrivate::QHexEditPrivate(QScrollArea *scrollarea, QScrollBar *vscrollbar, QWidget *parent): QWidget(parent)
{
    this->_hexeditdatamanager = NULL;
    this->_scrollarea = scrollarea;
    this->_vscrollbar = vscrollbar;
    this->_blink = false;
    this->_readonly = false;
    this->_selpart = HexPart;
    this->_cursorX = this->_cursorY = 0;
    this->_charheight = this->_charwidth = 0;

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
    if(this->_hexeditdatamanager->undo())
        this->update();
}

void QHexEditPrivate::redo()
{
    if(this->_hexeditdatamanager->redo())
        this->update();
}

void QHexEditPrivate::cut()
{
    if(this->_hexeditdatamanager->cut())
    {
        this->adjust();
        this->ensureVisible();
    }
}

void QHexEditPrivate::copy()
{
    this->_hexeditdatamanager->copy();
}

void QHexEditPrivate::paste()
{
    if(this->_hexeditdatamanager->paste())
    {
        this->adjust();
        this->ensureVisible();
    }
}

void QHexEditPrivate::doAnd(qint64 start, qint64 end, uchar value)
{
    if(this->_hexeditdatamanager)
    {
        this->_hexeditdatamanager->doAnd(start, end, value);
        this->update();
    }
}

void QHexEditPrivate::doOr(qint64 start, qint64 end, uchar value)
{
    if(this->_hexeditdatamanager)
    {
        this->_hexeditdatamanager->doOr(start, end, value);
        this->update();
    }
}

void QHexEditPrivate::doXor(qint64 start, qint64 end, uchar value)
{
    if(this->_hexeditdatamanager)
    {
        this->_hexeditdatamanager->doXor(start, end, value);
        this->update();
    }
}

void QHexEditPrivate::doMod(qint64 start, qint64 end, uchar value)
{
    if(this->_hexeditdatamanager)
    {
        this->_hexeditdatamanager->doMod(start, end, value);
        this->update();
    }
}

void QHexEditPrivate::doNot(qint64 start, qint64 end)
{
    if(this->_hexeditdatamanager)
    {
        this->_hexeditdatamanager->doNot(start, end);
        this->update();
    }
}

void QHexEditPrivate::setData(QHexEditData *hexeditdata)
{
    if(this->_hexeditdatamanager)
        disconnect(this->_hexeditdatamanager->data(), SIGNAL(dataChanged(qint64,qint64)), this, SLOT(hexEditDataChanged(qint64,qint64))); /* Disconnect previous HexEditData, if any */

    if(hexeditdata)
    {
        connect(hexeditdata, SIGNAL(dataChanged(qint64,qint64)), SLOT(hexEditDataChanged(qint64,qint64)));
        this->_hexeditdatamanager = new QHexEditDataManager(hexeditdata);

        /* Check Max Address Width */
        QString addrString = QString("%1").arg(this->_hexeditdatamanager->length() / QHexEditDataManager::BYTES_PER_LINE);

        if(this->_addressWidth < addrString.length()) /* Adjust Address Width */
            this->_addressWidth = addrString.length();

        this->setCursorPos(0); /* Make a Selection */
    }
    else
        this->_hexeditdatamanager = NULL;

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
    {
        this->_vscrollbar->setValue((currline + 1) - vislines);
        this->update();
    }
}

void QHexEditPrivate::adjust()
{
    QFontMetrics fm = this->fontMetrics();

    this->_charwidth = fm.width(" ");
    this->_charheight = fm.height();

    this->_xposhex =  this->_charwidth * (this->_addressWidth + 1);
    this->_xposascii = this->_xposhex + (this->_charwidth * (QHexEditDataManager::BYTES_PER_LINE * 3));
    this->_xPosend = this->_xposascii + (this->_charwidth * QHexEditDataManager::BYTES_PER_LINE);

    if(this->_hexeditdatamanager)
    {
        /* Setup ScrollBars */
        qint64 totLines = this->_hexeditdatamanager->length() / QHexEditDataManager::BYTES_PER_LINE;
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

    this->update();
}

void QHexEditPrivate::setSelectionEnd(qint64 pos, int charidx)
{
    qint64 oldselend = this->_hexeditdatamanager->selectionEnd();

    this->_hexeditdatamanager->setSelectionEnd(pos, charidx);
    this->setCursorXY(this->_hexeditdatamanager->selectionEnd(), charidx);
    this->update();

    if(oldselend != pos)
        emit selectionChanged(qMax(this->_hexeditdatamanager->selectionStart(), pos) - qMin(this->_hexeditdatamanager->selectionStart(), pos));
}

void QHexEditPrivate::setSelection(qint64 start, qint64 end)
{
    if(end == -1)
        end = this->_hexeditdatamanager->length();

    this->_hexeditdatamanager->setSelectionStart(start);
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
    if(this->_hexeditdatamanager)
        return this->_hexeditdatamanager->indexOf(ba, start);

    return -1;
}

bool QHexEditPrivate::readOnly()
{
    return this->_readonly;
}

int QHexEditPrivate::addressWidth()
{
    return this->_addressWidth;
}

int QHexEditPrivate::wheelScrollLines()
{
    return this->_whellscrolllines;
}

qint64 QHexEditPrivate::cursorPos()
{
    if(this->_hexeditdatamanager)
        return this->_hexeditdatamanager->cursorPos();

    return 0;
}

qint64 QHexEditPrivate::selectionStart()
{
    if(this->_hexeditdatamanager)
        return this->_hexeditdatamanager->selectionStart();

    return 0;
}

qint64 QHexEditPrivate::selectionEnd()
{
    if(this->_hexeditdatamanager)
        return this->_hexeditdatamanager->selectionEnd();

    return 0;
}

QHexEditData *QHexEditPrivate::data()
{
    if(this->_hexeditdatamanager)
        return this->_hexeditdatamanager->data();

    return NULL;
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

QColor QHexEditPrivate::byteWeight(uchar b)
{
    QColor c = this->palette().color(QPalette::WindowText);

    if(!b)
        c = QColor(Qt::darkGray);

    return c;
}

void QHexEditPrivate::setCursorPos(qint64 pos, int charidx)
{
    if(pos < 0)
        pos = 0;
    else if(pos > this->_hexeditdatamanager->length())
        pos = this->_hexeditdatamanager->length();

    qint64 oldpos = this->_hexeditdatamanager->cursorPos();
    int oldcharidx = this->_hexeditdatamanager->charIndex();
    qint64 oldsellength = this->_hexeditdatamanager->selectionEnd() - this->_hexeditdatamanager->selectionStart();

    /* Delete Cursor */
    this->_blink = false;
    this->update();

    this->_hexeditdatamanager->setCursorPos(pos, charidx);
    this->setCursorXY(this->_hexeditdatamanager->cursorPos(), charidx);

    /* Redraw Cursor NOW! */
    this->_blink = true;
    this->update();

    if(oldsellength != (this->_hexeditdatamanager->selectionEnd() - this->_hexeditdatamanager->selectionStart()))
        emit selectionChanged(0);

    if(oldpos != pos || oldcharidx != charidx)
        emit positionChanged(pos);
}

void QHexEditPrivate::setCursorXY(qint64 pos, int charidx)
{
    this->_cursorY = ((pos - (this->verticalSliderPosition64() * QHexEditDataManager::BYTES_PER_LINE)) / QHexEditDataManager::BYTES_PER_LINE) * this->_charheight;

    if(this->_selpart == AddressPart || this->_selpart == HexPart)
    {
        qint64 x = pos % QHexEditDataManager::BYTES_PER_LINE;
        this->_cursorX = x * (3 * this->_charwidth) + this->_xposhex;

        if(charidx)
            this->_cursorX += this->_charwidth;
    }
    else /* if this->_selPart == AsciiPart */
    {
        qint64 x = pos % QHexEditDataManager::BYTES_PER_LINE;
        this->_cursorX = x * this->_charwidth + this->_xposascii;
    }

    this->ensureVisible();
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
    int lineMax = this->_hexeditdatamanager->length() / QHexEditDataManager::BYTES_PER_LINE;
    painter.fillRect(0, y, this->_xposhex - (this->_charwidth / 2), this->_charheight, this->_addressbackcolor);

    if(line <= lineMax)
    {
        QString addr = QString("%1").arg(line * QHexEditDataManager::BYTES_PER_LINE, this->_addressWidth, 16, QLatin1Char('0')).toUpper();

        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(this->_addressforecolor);
        painter.drawText(0, y, fm.width(addr), this->_charheight, Qt::AlignLeft | Qt::AlignTop, addr);
    }
}

void QHexEditPrivate::drawHexPart(QPainter &painter, QFontMetrics& fm, qint64 line, int y)
{
    int x = this->_xposhex;
    qint64 lineLength = QHexEditDataManager::BYTES_PER_LINE;

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(this->palette().base());

    qint64 lineStart = line * QHexEditDataManager::BYTES_PER_LINE;
    qint64 lineEnd = lineStart + QHexEditDataManager::BYTES_PER_LINE;

    if(this->_hexeditdatamanager->cursorPos() >= lineStart && this->_hexeditdatamanager->cursorPos() < lineEnd) /* This is the Selected Line! */
        painter.fillRect(this->_xposhex - (this->_charwidth / 2), y, this->_xposascii - this->_xposhex, this->_charheight, this->_selLineColor);
    else if(line & 1)
        painter.fillRect(this->_xposhex - (this->_charwidth / 2), y, this->_xposascii - this->_xposhex, this->_charheight, this->_alternatelinecolor);

    for(qint64 i = 0; i < lineLength; i++)
    {
        bool highlighted = false;
        qint64 pos = (line * lineLength) + i;

        if(pos >= this->_hexeditdatamanager->length())
            return; /* Reached EOF */

        QByteArray ba = this->_hexeditdatamanager->read(lineStart + i, 1);
        QString s = QString(ba.toHex()).toUpper();
        int w = fm.width(s);

        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(this->byteWeight(static_cast<uchar>(ba[0])));

        if(pos == this->_hexeditdatamanager->selectionStart() && (this->_selpart == AsciiPart || !this->hasFocus()))
        {
            highlighted = true;
            w += this->_charwidth;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(this->_selcursorbrush);
        }

        if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd()) /* Highlight Selection */
        {
            qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
            qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

            if((pos >= start && pos < end))
            {
                if(i < (lineLength - 1))
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
            if(i < (lineLength - 1))
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
    qint64 lineStart = line * QHexEditDataManager::BYTES_PER_LINE;
    qint64 lineEnd = lineStart + QHexEditDataManager::BYTES_PER_LINE;

    if(this->_hexeditdatamanager->cursorPos() >= lineStart && this->_hexeditdatamanager->cursorPos() < lineEnd) /* This is the Selected Line! */
        painter.fillRect(this->_xposascii - span, y, (this->_xPosend - this->_xposascii) + this->_charwidth, this->_charheight, this->_selLineColor);
    else if(line & 1)
        painter.fillRect(this->_xposascii - span, y, (this->_xPosend - this->_xposascii) + this->_charwidth, this->_charheight, this->_alternatelinecolor);

    for(qint64 i = 0; i < QHexEditDataManager::BYTES_PER_LINE; i++)
    {
        bool highlighted = false;
        qint64 pos = (line * QHexEditDataManager::BYTES_PER_LINE) + i;

        if(pos >= this->_hexeditdatamanager->length())
            return; /* Reached EOF */

        QByteArray ba = this->_hexeditdatamanager->read(lineStart + i, 1);
        QString s = QString(ba);
        uchar ch = ba.at(0);

        if(pos == this->_hexeditdatamanager->selectionStart() && (this->_selpart == HexPart || !this->hasFocus()))
        {
            highlighted = true;
            painter.setBackgroundMode(Qt::OpaqueMode);
            painter.setBackground(this->_selcursorbrush);
        }

        if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd()) /* Highlight Selection */
        {
            qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
            qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

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

        if(ch >= (uchar)0x20 && ch <= (uchar)0x7F)
        {
            w = fm.width(s);
            painter.drawText(x, y, w, this->_charheight, Qt::AlignLeft | Qt::AlignTop, s);
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
    qint64 y = (this->verticalSliderPosition64() + (pt.y() / this->_charheight)) * QHexEditDataManager::BYTES_PER_LINE, x = 0;
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

    if(this->_hexeditdatamanager)
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

        int span = this->_charwidth / 2;
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(this->palette().color(QPalette::WindowText));
        painter.drawLine(this->_xposhex - span, 0, this->_xposhex - span, this->height());
        painter.drawLine(this->_xposascii - span, 0, this->_xposascii - span, this->height());
        painter.drawLine(this->_xPosend + span, 0, this->_xPosend + span, this->height());
    }

    if(this->_hexeditdatamanager && this->hasFocus() && this->_blink)
    {
        if(this->_hexeditdatamanager->mode() == QHexEditDataManager::Overwrite)
            painter.fillRect(this->_cursorX, this->_cursorY + this->_charheight - 3, this->_charwidth, 2, this->palette().color(QPalette::WindowText));
        else
            painter.fillRect(this->_cursorX, this->_cursorY, 2, this->_charheight, this->palette().color(QPalette::WindowText));
    }
}

void QHexEditPrivate::mousePressEvent(QMouseEvent* event)
{
    if(this->_hexeditdatamanager)
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
    if(this->_hexeditdatamanager)
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
    if(this->_hexeditdatamanager->length())
    {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;

        if(event->orientation() == Qt::Vertical)
        {
            int pos = this->verticalSliderPosition64() - (numSteps * this->_whellscrolllines);
            int maxlines = this->_hexeditdatamanager->length() / QHexEditDataManager::BYTES_PER_LINE;

            /* Bounds Check */
            if(pos < 0)
                pos = 0;
            else if(pos > maxlines)
                pos = maxlines;

            this->_vscrollbar->setSliderPosition(pos);
            this->update();

            event->accept();
        }
    }
    else
        event->ignore();
}

void QHexEditPrivate::keyPressEvent(QKeyEvent* event)
{
    if(this->_hexeditdatamanager)
    {
        /* Move Events */
        if(event->matches(QKeySequence::MoveToNextChar) || event->matches(QKeySequence::MoveToPreviousChar))
        {
            if(this->_selpart == HexPart)
            {
                if(event->matches(QKeySequence::MoveToNextChar))
                {
                    if(this->_hexeditdatamanager->charIndex() == 0)
                        this->setCursorPos(this->_hexeditdatamanager->selectionStart(), 1);
                    else /* if(this->_hexeditbuffer->charIndex() == 1) */
                        this->setCursorPos(this->_hexeditdatamanager->selectionStart() + 1, 0);
                }
                else if(event->matches(QKeySequence::MoveToPreviousChar))
                {
                    if(this->_hexeditdatamanager->charIndex() == 0)
                        this->setCursorPos(this->_hexeditdatamanager->selectionStart() - 1, 1);
                    else /* if(this->_hexeditbuffer->charIndex() == 1) */
                        this->setCursorPos(this->_hexeditdatamanager->selectionStart(), 0);
                }
            }
            else if(this->_selpart == AsciiPart)
            {
                if(event->matches(QKeySequence::MoveToNextChar))
                    this->setCursorPos(this->_hexeditdatamanager->selectionStart() + 1, 0);
                else if(event->matches(QKeySequence::MoveToPreviousChar))
                    this->setCursorPos(this->_hexeditdatamanager->selectionStart() - 1, 0);
            }
        }
        else if(event->matches(QKeySequence::MoveToNextLine))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() + QHexEditDataManager::BYTES_PER_LINE, this->_hexeditdatamanager->charIndex());
        else if(event->matches(QKeySequence::MoveToPreviousLine))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() - QHexEditDataManager::BYTES_PER_LINE, this->_hexeditdatamanager->charIndex());
        else if(event->matches(QKeySequence::MoveToStartOfLine))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() - (this->_hexeditdatamanager->selectionStart() % QHexEditDataManager::BYTES_PER_LINE));
        else if(event->matches(QKeySequence::MoveToNextPage))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() + (((this->height() / this->_charheight) - 1) * QHexEditDataManager::BYTES_PER_LINE));
        else if(event->matches(QKeySequence::MoveToPreviousPage))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() - (((this->height() / this->_charheight) - 1) * QHexEditDataManager::BYTES_PER_LINE));
        else if(event->matches(QKeySequence::MoveToEndOfLine))
            this->setCursorPos(this->_hexeditdatamanager->selectionStart() | (QHexEditDataManager::BYTES_PER_LINE - 1));
        else if(event->matches(QKeySequence::MoveToStartOfDocument))
            this->setCursorPos(0);
        else if(event->matches(QKeySequence::MoveToEndOfDocument))
            this->setCursorPos(this->_hexeditdatamanager->length());

        /* Select Events */
        if(event->matches(QKeySequence::SelectNextChar) || event->matches(QKeySequence::SelectPreviousChar))
        {
            if(this->_selpart == HexPart)
            {
                if(event->matches(QKeySequence::SelectNextChar))
                {
                    if(this->_hexeditdatamanager->charIndex() == 0)
                        this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd(), 1);
                    else /* if(this->_hexeditbuffer->charIndex() == 1) */
                        this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() + 1, 0);
                }
                else if(event->matches(QKeySequence::SelectPreviousChar))
                {
                    if(this->_hexeditdatamanager->charIndex() == 0)
                        this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() - 1, 1);
                    else /* if(this->_hexeditbuffer->charIndex() == 1) */
                        this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd(), 0);
                }
            }
            else if(this->_selpart == AsciiPart)
            {
                if(event->matches(QKeySequence::SelectNextChar))
                    this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() + 1, 0);
                else if(event->matches(QKeySequence::SelectPreviousChar))
                    this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() - 1, 0);
            }
        }
        else if(event->matches(QKeySequence::SelectNextLine))
            this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() + QHexEditDataManager::BYTES_PER_LINE, this->_hexeditdatamanager->charIndex());
        else if(event->matches(QKeySequence::SelectPreviousLine))
            this->setSelectionEnd(this->_hexeditdatamanager->selectionEnd() - QHexEditDataManager::BYTES_PER_LINE, this->_hexeditdatamanager->charIndex());
        else if(event->matches(QKeySequence::SelectStartOfDocument))
            this->setSelection(this->_hexeditdatamanager->selectionEnd(), 0);
        else if(event->matches(QKeySequence::SelectEndOfDocument))
            this->setSelectionEnd(this->_hexeditdatamanager->length(), 0);
        else if(event->matches(QKeySequence::SelectAll))
            this->setSelection(0, this->_hexeditdatamanager->length());

        if(!this->_readonly)
        {
            /* Text Input Commands */
            int key = (int)event->text()[0].toLatin1();

            if(this->_selpart == HexPart)
            {
                if((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f'))
                {
                    /* Remove Selected Text */
                    if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd())
                    {
                        qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
                        qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

                        this->_hexeditdatamanager->remove(start, end - start);
                        this->setCursorPos(start);  //FIXME: Check charIndex here?
                    }

                    /* Insert Mode: Add one byte in current position */
                    if(this->_hexeditdatamanager->mode() != QHexEditDataManager::Overwrite)
                    {
                        if(!(this->_hexeditdatamanager->charIndex() == 1))
                            this->_hexeditdatamanager->insert(this->_hexeditdatamanager->selectionStart(), (char)0, false);

                        qint64 currSel = this->_hexeditdatamanager->selectionStart() + 1;

                        /* A New Line is added at EOF: update scrollbar information */
                        if(!(currSel % QHexEditDataManager::BYTES_PER_LINE) && (currSel >= this->_hexeditdatamanager->length()))
                            this->adjust();
                    }

                    /* Update Byte's Nibble */
                    if((this->_hexeditdatamanager->length() > 0) && (this->_hexeditdatamanager->selectionStart() < this->_hexeditdatamanager->length()))
                    {
                        /* Convert digit's string rappresentation to integer rappresentation */
                        uchar val = (unsigned char)QString(key).toUInt(NULL, 16);

                        QByteArray hexVal = this->_hexeditdatamanager->read(this->_hexeditdatamanager->selectionStart(), 1);

                        if(this->_hexeditdatamanager->charIndex() == 1) /* Change Byte's Low Part */
                            hexVal[0] = (hexVal[0] & 0xF0) + val;
                        else /* Change Byte's High Part */
                            hexVal[0] = (hexVal[0] & 0x0F) + (val << 4);

                        //this->_hexeditbuffer->replace(this->_hexeditbuffer->selectionStart(), hexVal);
                        this->_hexeditdatamanager->remove(this->_hexeditdatamanager->selectionStart(), 1, false); /* It's an intermediate passage! */
                        this->_hexeditdatamanager->insert(this->_hexeditdatamanager->selectionStart(), hexVal);

                        emit bytesChanged(this->_hexeditdatamanager->selectionStart());
                        this->adjust(); /* One or More Bytes Removed: Update ScrollBar Information */

                        if(this->_hexeditdatamanager->charIndex() == 1)
                            this->setCursorPos(this->_hexeditdatamanager->selectionStart() + 1, 0); //FIXME: Check charIndex here?
                        else
                            this->setCursorPos(this->_hexeditdatamanager->selectionStart(), 1);
                    }
                }
            }
            else if(this->_selpart == AsciiPart)
            {
                if(key >= 0x20 && key <= 0x7E) /* Check if is a printable Char */
                {
                    /* Remove Selected Text */
                    if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd())
                    {
                        qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
                        qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

                        this->_hexeditdatamanager->remove(start, end - start);
                        this->setCursorPos(start, 0);
                    }

                    /* Insert Mode: Add one byte in current position */
                    if(this->_hexeditdatamanager->mode() == QHexEditDataManager::Insert)
                    {
                        this->_hexeditdatamanager->insert(this->_hexeditdatamanager->selectionStart(), (char)key);
                        qint64 currSel = this->_hexeditdatamanager->selectionStart() + 1;

                        emit bytesChanged(this->_hexeditdatamanager->selectionStart());

                        /* A New Row is added at EOF, update scrollbar information */
                        if(!(currSel % QHexEditDataManager::BYTES_PER_LINE) && (currSel >= this->_hexeditdatamanager->length()))
                            this->adjust();

                        this->setCursorPos(currSel, 0);
                    }
                    else if(this->_hexeditdatamanager->mode() == QHexEditDataManager::Overwrite && ((this->_hexeditdatamanager->length() > 0) && (this->_hexeditdatamanager->selectionStart() < this->_hexeditdatamanager->length())))
                    {
                        this->_hexeditdatamanager->replace(this->_hexeditdatamanager->selectionStart(), (char)key);

                        emit bytesChanged(this->_hexeditdatamanager->selectionStart());
                        this->adjust(); /* One or More Bytes Removed: Update ScrollBar Information */
                        this->setCursorPos(this->_hexeditdatamanager->selectionStart() + 1, 0);
                    }
                }
            }

            /* Char Deletion Events */
            if(event->key() == Qt::Key_Delete)
            {
                /* Remove Selected Text */
                if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd())
                {
                    qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
                    qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

                    this->_hexeditdatamanager->remove(start, end - start);
                    this->setCursorPos(start, 0);
                }
                else
                {
                    if(this->_hexeditdatamanager->mode() == QHexEditDataManager::Overwrite)
                    {
                        if(this->_selpart == HexPart)
                        {
                            QByteArray hexVal = this->_hexeditdatamanager->read(this->_hexeditdatamanager->selectionStart(), 1);

                            if(this->_hexeditdatamanager->charIndex() == 1) /* Change Byte's Low Part */
                                hexVal[0] = (hexVal[0] & 0xF0);
                            else /* Change Byte's High Part */
                                hexVal[0] = (hexVal[0] & 0x0F);

                            this->_hexeditdatamanager->replace(this->_hexeditdatamanager->selectionStart(), hexVal);
                        }
                        else
                            this->_hexeditdatamanager->replace(this->_hexeditdatamanager->selectionStart(), (char)0);
                    }
                    else
                        this->_hexeditdatamanager->remove(this->_hexeditdatamanager->selectionStart(), 1);
                }

                emit bytesChanged(this->_hexeditdatamanager->selectionStart());
                this->adjust(); /* One or More Bytes Removed: Update ScrollBar Information */
            }
            else if((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
            {
                if(this->_hexeditdatamanager->selectionStart() != this->_hexeditdatamanager->selectionEnd())
                {
                    qint64 start = qMin(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());
                    qint64 end = qMax(this->_hexeditdatamanager->selectionStart(), this->_hexeditdatamanager->selectionEnd());

                    this->_hexeditdatamanager->remove(start, end - start);
                    this->setCursorPos(start, 0);
                }
                else
                {
                    if(this->_hexeditdatamanager->selectionStart())
                    {
                        if(this->_hexeditdatamanager->mode() == QHexEditDataManager::Overwrite)
                            this->_hexeditdatamanager->replace(this->_hexeditdatamanager->selectionStart() - 1, (char)0);
                        else
                            this->_hexeditdatamanager->remove(this->_hexeditdatamanager->selectionStart() - 1, 1);

                        this->setCursorPos(this->_hexeditdatamanager->selectionStart() - 1);
                    }
                }

                emit bytesChanged(this->_hexeditdatamanager->selectionStart());
                this->adjust(); /* One or More Bytes Removed: Update ScrollBar Information */
            }
        }

        /* No Modifier Keys */
        if((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
            this->_hexeditdatamanager->setMode(this->_hexeditdatamanager->mode() == QHexEditDataManager::Overwrite ? QHexEditDataManager::Insert : QHexEditDataManager::Overwrite);

        /* Undo/Redo Keys */
        if(event->matches(QKeySequence::Undo))
            this->undo();
        else if(event->matches(QKeySequence::Redo))
            this->redo();

        /* Clipboard Keys */
        if(event->matches(QKeySequence::Cut))
            this->cut();
        else if(event->matches(QKeySequence::Copy))
            this->copy();
        else if(event->matches(QKeySequence::Paste))
            this->paste();

        this->ensureVisible();
        this->update();
    }
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

void QHexEditPrivate::hexEditDataChanged(qint64, qint64)
{
    this->update();
    this->adjust();  /* Update ScrollBars */
}
