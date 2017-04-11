#include "qhexmetadataitem.h"

QHexMetadataItem::QHexMetadataItem(integer_t startoffset, integer_t endoffset, QObject *parent) : QObject(parent), _startoffset(startoffset), _endoffset(endoffset)
{

}

integer_t QHexMetadataItem::startOffset() const
{
    return this->_startoffset;
}

integer_t QHexMetadataItem::endOffset() const
{
    return this->_endoffset;
}

bool QHexMetadataItem::contains(integer_t offset) const
{
    return (offset >= this->_startoffset) && (offset <= this->_endoffset);
}

bool QHexMetadataItem::hasBackColor() const
{
    return this->_backcolor.isValid();
}

bool QHexMetadataItem::hasForeColor() const
{
    return this->_forecolor.isValid();
}

bool QHexMetadataItem::hasComment() const
{
    return !this->_comment.isEmpty();
}

const QColor& QHexMetadataItem::backColor() const
{
    return this->_backcolor;
}

const QColor &QHexMetadataItem::foreColor() const
{
    return this->_forecolor;
}

const QString &QHexMetadataItem::comment() const
{
    return this->_comment;
}

void QHexMetadataItem::setBackColor(const QColor &c)
{
    this->_backcolor = c;
}

void QHexMetadataItem::setForeColor(const QColor &c)
{
    this->_forecolor = c;
}

void QHexMetadataItem::setComment(const QString &s)
{
    this->_comment = s;
}

void QHexMetadataItem::clearComment()
{
    this->_comment.clear();
}

void QHexMetadataItem::clearColors()
{
    this->_backcolor = QColor();
    this->_forecolor = QColor();
}
