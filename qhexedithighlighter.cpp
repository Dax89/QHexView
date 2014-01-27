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
    ColorRange cr = this->rangeAt(this->_backgroundranges, pos);

    if(!cr.isValid())
        return this->_backdefault;

    return cr.color();
}

QColor QHexEditHighlighter::foreColor(qint64 pos) const
{
    ColorRange cr = this->rangeAt(this->_foregroundranges, pos);

    if(!cr.isValid())
        return this->_foredefault;

    return cr.color();
}

void QHexEditHighlighter::highlightForeground(qint64 start, qint64 end, const QColor& color)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->internalHighlight(this->_foregroundranges, start, end, color);
}

void QHexEditHighlighter::highlightBackground(qint64 start, qint64 end, const QColor& color)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->internalHighlight(this->_backgroundranges, start, end, color);
}

void QHexEditHighlighter::clearHighlight(qint64 start, qint64 end)
{
    if(start < 0)
        start = 0;

    if(end > this->_hexeditdata->length())
        end = this->_hexeditdata->length() - 1;

    this->internalClear(this->_foregroundranges, start, end);
    this->internalClear(this->_backgroundranges, start, end);
}

bool QHexEditHighlighter::canOptimize(const QHexEditHighlighter::ColorRange cr, qint64 start, qint64 end, const QColor &color) const
{
    return ((cr.pos() == start) && (cr.endPos() == end)) || ((cr.color() == color) && (((start - cr.endPos()) == 1) || ((cr.pos() - end) == 1)));
}

QHexEditHighlighter::ColorRange QHexEditHighlighter::rangeAt(const QHexEditHighlighter::ColorList &rangelist, qint64 pos, qint64 *index) const
{
    ColorRange cr = ColorRange::invalid();

    for(qint64 i = 0; i < rangelist.length(); i++)
    {
        cr = rangelist[i];

        if(cr.contains(pos))
        {
            if(index)
                *index = i;

            return cr;
        }
    }

    return ColorRange::invalid();
}

QHexEditHighlighter::ColorRange QHexEditHighlighter::insertionPoint(const QHexEditHighlighter::ColorList& rangelist, qint64 pos, qint64 *index) const
{    
    qint64 i = -1;
    ColorRange cr = ColorRange::invalid();

    for(i = 0; i < rangelist.length(); i++)
    {
        cr = rangelist[i];

        if((cr.pos() > pos) || cr.contains(pos))
        {
            if(index)
                *index = i;

            return cr;
        }
    }

    if(cr.isValid() && (pos == cr.endPos())) /* Return Last Item for Append */
    {
        if(index)
            *index = i;

        return cr;
    }

    return ColorRange::invalid();
}

void QHexEditHighlighter::internalClear(QHexEditHighlighter::ColorList& rangelist, qint64 start, qint64 end)
{
    qint64 i = -1;
    ColorRange cr = this->rangeAt(rangelist, start, &i);

    if(!cr.isValid())
        return;

    QHexEditHighlighter::ColorList olditems, newitems;
    qint64 len = (end - start) + 1;
    qint64 remoffset = start - cr.pos(), remlength = len;

    if(remoffset)
    {
        newitems.append(ColorRange(cr.pos(), remoffset, cr.color()));

        if((remoffset + remlength) < cr.length())
            newitems.append(ColorRange(cr.pos() + remoffset + remlength, cr.length() - remoffset - remlength, cr.color()));

        remlength -= qMin(remlength, cr.length() - remoffset);
        olditems.append(cr);
        i++;
    }

    while(remlength && (i < rangelist.length()))
    {
        cr = rangelist[i];

        if(remlength < cr.length())
            newitems.append(ColorRange(cr.pos() + remlength, cr.length() - remlength, cr.color()));

        remlength -= qMin(remlength, cr.length());
        olditems.append(cr);
        i++;
    }

    qint64 remidx = i - olditems.length();
    rangelist.erase(rangelist.begin() + remidx, rangelist.begin() + (remidx + olditems.length()));

    for(qint64 j = 0; j < newitems.length(); j++)
        rangelist.insert(remidx + j, newitems.at(j));
}

void QHexEditHighlighter::internalHighlight(QHexEditHighlighter::ColorList &rangelist, qint64 start, qint64 end, const QColor& color)
{
    qint64 len = end - start;
    this->internalClear(rangelist, start, end);

    qint64 i = -1;
    ColorRange cr = this->insertionPoint(rangelist, start, &i);

    if(!cr.isValid())
    {
        rangelist.append(ColorRange(start, len, color));
        return;
    }

    QHexEditHighlighter::ColorList olditems, newitems;
    qint64 modoffset = start - cr.pos();

    if(this->canOptimize(cr, start, end, color))
    {
        if((start == cr.pos()) && (end == cr.endPos())) /* Attempting to insert an item with the same range, just change the color */
        {
            rangelist[i].setColor(color);
            return;
        }
        else if(cr.color() == color)
        {
            /* Merge old and new item, because it's the same color and they are adjacent */

            if((cr.endPos() - start) == 1) /* Case: OldItem | NewItem */
            {
                olditems.append(cr);
                newitems.append(ColorRange(cr.pos(), end, color));
            }
            else if((cr.pos() - end) == 1) /* Case: NewItem | OldItem */
            {
                olditems.append(cr);
                newitems.append(ColorRange(start, cr.endPos(), color));
            }
        }
    }   
    else if(!modoffset || (end < cr.pos()))
        newitems.append(ColorRange(start, len, color));
    else
    {
        olditems.append(cr);
        newitems.append(ColorRange(cr.pos(), modoffset, cr.color()));
        newitems.append(ColorRange(start, len, color));
        newitems.append(ColorRange(cr.pos() + modoffset, cr.length() - modoffset, cr.color()));
    }

    for(qint64 j = 0; j < olditems.length(); j++)
        rangelist.removeAt(i);

    for(qint64 j = 0; j < newitems.length(); j++)
        rangelist.insert(i + j, newitems[j]);
}
