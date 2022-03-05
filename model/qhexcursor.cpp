#include "qhexcursor.h"
#include "qhexdocument.h"

/*
 * https://stackoverflow.com/questions/10803043/inverse-column-row-major-order-transformation
 *
 *  If the index is calculated as:
 *      offset = row + column*NUMROWS
 *  then the inverse would be:
 *      row = offset % NUMROWS
 *      column = offset / NUMROWS
 *  where % is modulus, and / is integer division.
 */

QHexCursor::QHexCursor(QHexDocument* parent) : QObject(parent) { }
QHexDocument* QHexCursor::document() const { return qobject_cast<QHexDocument*>(this->parent()); }
QHexCursor::Mode QHexCursor::mode() const { return m_mode; }
qint64 QHexCursor::offset() const { return this->positionToOffset(m_position); }
qint64 QHexCursor::selectionStartOffset() const { return this->positionToOffset(this->selectionStart()); }
qint64 QHexCursor::selectionEndOffset() const { return this->positionToOffset(this->selectionEnd()); }
qint64 QHexCursor::line() const { return m_position.line; }
qint64 QHexCursor::column() const { return m_position.column; }

QHexCursor::Position QHexCursor::selectionStart() const
{
    if(m_position.line < m_selection.line)
        return m_position;

    if(m_position.line == m_selection.line)
    {
        if(m_position.column < m_selection.column)
            return m_position;
    }

    return m_selection;
}

QHexCursor::Position QHexCursor::selectionEnd() const
{
    if(m_position.line > m_selection.line)
        return m_position;

    if(m_position.line == m_selection.line)
    {
        if(m_position.column > m_selection.column)
            return m_position;
    }

    return m_selection;
}

qint64 QHexCursor::selectionLength() const
{
    auto selstart = this->selectionStartOffset(), selend = this->selectionEndOffset();
    return selstart == selend ? 0 : selend - selstart + 1;
}

QHexCursor::Position QHexCursor::position() const { return m_position; }
QByteArray QHexCursor::selectedBytes() const { return this->hasSelection() ? this->document()->read(this->selectionStartOffset(), this->selectionLength()) : QByteArray{ }; }
bool QHexCursor::hasSelection() const { return m_position != m_selection; }

bool QHexCursor::isSelected(qint64 line, qint64 column) const
{
    if(!this->hasSelection()) return false;

    auto selstart = this->selectionStart(), selend = this->selectionEnd();
    if(line > selstart.line && line < selend.line) return true;
    if(line == selstart.line && line == selend.line) return column >= selstart.column && column <= selend.column;
    if(line == selstart.line) return column >= selstart.column;
    if(line == selend.line) return column <= selend.column;
    return false;
}

void QHexCursor::setMode(Mode m)
{
    if(m_mode == m) return;
    m_mode = m;
    Q_EMIT modeChanged();
}

void QHexCursor::toggleMode()
{
    switch(m_mode)
    {
        case Mode::Insert: this->setMode(Mode::Overwrite); break;
        case Mode::Overwrite: this->setMode(Mode::Insert); break;
    }
}

void QHexCursor::move(qint64 offset) { this->move(this->offsetToPosition(offset)); }
void QHexCursor::move(qint64 line, qint64 column) { return this->move({line, column}); }

void QHexCursor::move(Position pos)
{
    if(pos.line >= 0) m_selection.line = pos.line;
    if(pos.column >= 0) m_selection.column = pos.column;
    this->select(pos);
}

void QHexCursor::select(qint64 offset) { this->select(this->offsetToPosition(offset)); }
void QHexCursor::select(qint64 line, qint64 column) { this->select({line, column}); }

void QHexCursor::select(Position pos)
{
    if(pos.line >= 0) m_position.line = pos.line;
    if(pos.column >= 0) m_position.column = pos.column;
    Q_EMIT positionChanged();
}

void QHexCursor::selectSize(qint64 length)
{
    auto* opt = this->document()->options();
    Position pos = m_position;

    if(length > 0)
    {
        for(qint64 i = 0; i < length; i++)
        {
            pos.column++;

            if(pos.column >= opt->linelength)
            {
                //TODO: Check if line == EOF
                pos.line++;
                pos.column = 0;
            }
        }
    }
    else if(length < 0)
    {
        for(qint64 i = length; i-- > 0; )
        {
            pos.column--;

            if(pos.column <= 0)
            {
                //TODO: Check if line == 0
                pos.line--;
                pos.column = opt->linelength - 1;
            }
        }
    }
    else
        return;

    this->select(pos);
}

void QHexCursor::selectAll() { this->document()->selectAll(); }

void QHexCursor::removeSelection()
{
    if(!this->hasSelection()) return;
    this->document()->remove(this->selectionStartOffset(), this->selectionLength() - 1);
    this->clearSelection();
}

void QHexCursor::clearSelection()
{
    m_position = m_selection;
    Q_EMIT positionChanged();
}

qint64 QHexCursor::positionToOffset(Position pos) const { return QHexCursor::positionToOffset(this->document()->options(), pos); }
QHexCursor::Position QHexCursor::offsetToPosition(qint64 offset) const { return QHexCursor::offsetToPosition(this->document()->options(), offset); }
qint64 QHexCursor::positionToOffset(const QHexOptions* options, Position pos) { return options->linelength * pos.line + pos.column; }
QHexCursor::Position QHexCursor::offsetToPosition(const QHexOptions* options, qint64 offset) { return { offset / options->linelength, offset % options->linelength }; }
