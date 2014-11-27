#include "qhexedit.h"

QHexEdit::QHexEdit(QWidget *parent): QFrame(parent)
{
    this->_vscrollbar = new QScrollBar(Qt::Vertical);
    this->_scrollarea = new QScrollArea();
    this->_hexedit_p = new QHexEditPrivate(this->_scrollarea, this->_vscrollbar);

    /* Forward QHexEditPrivate's Signals */
    connect(this->_hexedit_p, SIGNAL(visibleLinesChanged()), this, SIGNAL(visibleLinesChanged()));
    connect(this->_hexedit_p, SIGNAL(positionChanged(qint64)), this, SIGNAL(positionChanged(qint64)));
    connect(this->_hexedit_p, SIGNAL(selectionChanged(qint64)), this, SIGNAL(selectionChanged(qint64)));
    connect(this->_hexedit_p, SIGNAL(verticalScrollBarValueChanged(int)), this, SIGNAL(verticalScrollBarValueChanged(int)));

    this->_scrollarea->setFocusPolicy(Qt::NoFocus);
    this->_scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); /* Do not show vertical QScrollBar!!! */
    this->_scrollarea->setFrameStyle(QFrame::NoFrame);
    this->_scrollarea->setWidgetResizable(true);
    this->_scrollarea->setWidget(this->_hexedit_p);

    this->setFocusPolicy(Qt::NoFocus);

    this->_hlayout = new QHBoxLayout();
    this->_hlayout->setSpacing(0);
    this->_hlayout->setMargin(0);
    this->_hlayout->addWidget(this->_scrollarea);
    this->_hlayout->addWidget(this->_vscrollbar);

    this->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    this->setLayout(this->_hlayout);
}

void QHexEdit::undo()
{
    this->_hexedit_p->undo();
}

void QHexEdit::setData(QHexEditData *hexeditdata)
{
    this->_hexedit_p->setData(hexeditdata);
}

void QHexEdit::selectPos(qint64 pos)
{
    this->setSelectionLength(pos, 1);
}

void QHexEdit::setSelection(qint64 start, qint64 end)
{
    this->_hexedit_p->setSelection(start, end);
}

void QHexEdit::setSelectionLength(qint64 start, qint64 length)
{
    this->setSelection(start, (start + length) - 1);
}

void QHexEdit::highlightBackground(qint64 start, qint64 end, const QColor &color)
{
    this->_hexedit_p->highlightBackground(start, end, color);
}

void QHexEdit::highlightForeground(qint64 start, qint64 end, const QColor &color)
{
    this->_hexedit_p->highlightForeground(start, end, color);
}

void QHexEdit::clearHighlight(qint64 start, qint64 end)
{
    this->_hexedit_p->clearHighlight(start, end);
}

void QHexEdit::clearHighlight()
{
    this->_hexedit_p->clearHighlight();
}

void QHexEdit::commentRange(qint64 start, qint64 end, const QString &comment)
{
    this->_hexedit_p->commentRange(start, end, comment);
}

void QHexEdit::uncommentRange(qint64 start, qint64 end)
{
    this->_hexedit_p->uncommentRange(start, end);
}

void QHexEdit::clearComments()
{
    this->_hexedit_p->clearComments();
}

void QHexEdit::setVerticalScrollBarValue(int value)
{
    this->_hexedit_p->setVerticalScrollBarValue(value);
}

void QHexEdit::scroll(QWheelEvent *event)
{
    this->_hexedit_p->scroll(event);
}

void QHexEdit::setCursorPos(qint64 pos)
{
    this->_hexedit_p->setCursorPos(pos);
}

void QHexEdit::paste()
{
    this->_hexedit_p->paste();
}

void QHexEdit::pasteHex()
{
    this->_hexedit_p->pasteHex();
}

void QHexEdit::selectAll()
{
    this->_hexedit_p->setSelection(0, -1);
}

int QHexEdit::addressWidth()
{
    return this->_hexedit_p->addressWidth();
}

int QHexEdit::visibleLinesCount()
{
    return this->_hexedit_p->visibleLinesCount();
}

QHexEditData *QHexEdit::data()
{
    return this->_hexedit_p->data();
}

qint64 QHexEdit::indexOf(QByteArray &ba, qint64 start)
{
    return this->_hexedit_p->indexOf(ba, start);
}

qint64 QHexEdit::baseAddress()
{
    return this->_hexedit_p->baseAddress();
}

qint64 QHexEdit::cursorPos()
{
    return this->_hexedit_p->cursorPos();
}

qint64 QHexEdit::selectionStart()
{
    return this->_hexedit_p->selectionStart();
}

qint64 QHexEdit::selectionEnd()
{
    return this->_hexedit_p->selectionEnd();
}

qint64 QHexEdit::selectionLength()
{
    return this->_hexedit_p->selectionEnd() - this->_hexedit_p->selectionStart();
}

void QHexEdit::setFont(const QFont &f)
{
    this->_hexedit_p->setFont(f);
}

void QHexEdit::setBaseAddress(qint64 ba)
{
    return this->_hexedit_p->setBaseAddress(ba);
}

qint64 QHexEdit::visibleStartOffset()
{
    return this->_hexedit_p->visibleStartOffset();
}

qint64 QHexEdit::visibleEndOffset()
{
    return this->_hexedit_p->visibleEndOffset();
}

bool QHexEdit::readOnly()
{
    return this->_hexedit_p->readOnly();
}

void QHexEdit::setReadOnly(bool b)
{
    this->_hexedit_p->setReadOnly(b);
}

void QHexEdit::copy()
{
    this->_hexedit_p->copy();
}

void QHexEdit::copyHex()
{
    this->_hexedit_p->copyHex();
}

void QHexEdit::cut()
{
    this->_hexedit_p->cut();
}

void QHexEdit::redo()
{
    this->_hexedit_p->redo();
}
