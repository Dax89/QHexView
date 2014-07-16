#include "qhexeditcomments.h"

QHexEditComments::QHexEditComments(QObject *parent): QObject(parent)
{

}

void QHexEditComments::commentRange(qint64 from, qint64 to, const QString &note)
{
    this->_notes.addRange(from, to, note);
}

void QHexEditComments::uncommentRange(qint64 from, qint64 to)
{
    this->_notes.clearRange(from, to);
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
    const QString* note = this->_notes.valueAt(offset);
    if(note)
        QToolTip::showText(pos, *note, qobject_cast<QWidget*>(this->parent()));
    else
        QToolTip::showText(pos, QString(), qobject_cast<QWidget*>(this->parent()));
}

void QHexEditComments::hideNote()
{
    QToolTip::hideText();
}
