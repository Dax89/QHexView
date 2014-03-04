#include "qhexeditcomments.h"

QHexEditComments::QHexEditComments(QObject *parent): QObject(parent)
{

}

void QHexEditComments::commentRange(qint64 from, qint64 to, const QString &note)
{
    for(qint64 i = from; i <= to; i++)
        this->_notes[i] = note;
}

void QHexEditComments::uncommentRange(qint64 from, qint64 to)
{
    for(qint64 i = from; i <= to; i++)
    {
        if(this->_notes.contains(i))
            this->_notes.remove(i);
    }
}

void QHexEditComments::clearComments()
{
    QToolTip::hideText();
    this->_notes.clear();
}

bool QHexEditComments::isCommented(qint64 offset)
{
    return this->_notes.contains(offset);
}

void QHexEditComments::displayNote(const QPoint& pos, qint64 offset)
{
    QToolTip::showText(pos, this->_notes[offset], qobject_cast<QWidget*>(this->parent()));
}

void QHexEditComments::hideNote()
{
    QToolTip::hideText();
}
