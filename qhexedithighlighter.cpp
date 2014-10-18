#include "qhexedithighlighter.h"

QHexEditHighlighter::QHexEditHighlighter(QHexEditData *hexeditdata, QColor backdefault, QColor foredefault, QObject *parent): QObject(parent), _hexeditdata(hexeditdata), _backdefault(backdefault), _foredefault(foredefault)
{

}

void QHexEditHighlighter::colors(qint64 pos, QColor &bc, QColor &fc) const
{
    bc = this->backColor(pos);
    fc = this->foreColor(pos);
}

QColor QHexEditHighlighter::defaultBackColor() const
{
    return this->_backdefault;
}

QColor QHexEditHighlighter::defaultForeColor() const
{
    return this->_foredefault;
}

QColor QHexEditHighlighter::backColor(qint64 pos) const
{
    if(!this->_backgroundmap.contains(pos))
        return this->_backdefault;

    return *this->_backgroundmap.valueAt(pos);
}

QColor QHexEditHighlighter::foreColor(qint64 pos) const
{
    if(!this->_foregroundmap.contains(pos))
        return this->_foredefault;

    return *this->_foregroundmap.valueAt(pos);
}

void QHexEditHighlighter::highlightForeground(qint64 start, qint64 end, const QColor& color)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->_foregroundmap.addRange(start, end, color);
}

void QHexEditHighlighter::highlightBackground(qint64 start, qint64 end, const QColor& color)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->_backgroundmap.addRange(start, end, color);
}

void QHexEditHighlighter::clearHighlight(qint64 start, qint64 end)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->_foregroundmap.clearRange(start, end);
    this->_backgroundmap.clearRange(start, end);
}
