#include "qhexmetadata.h"

QHexMetadata::QHexMetadata(integer_t startoffset, integer_t endoffset, QObject *parent) : QObject(parent), _startoffset(startoffset), _endoffset(endoffset)
{

}

integer_t QHexMetadata::startOffset() const
{
    return this->_startoffset;
}

integer_t QHexMetadata::endOffset() const
{
    return this->_endoffset;
}

bool QHexMetadata::contains(integer_t offset) const
{
    return (offset >= this->_startoffset) && (offset <= this->_endoffset);
}

bool QHexMetadata::hasBackColor() const
{
    return this->_backcolor.isValid();
}

bool QHexMetadata::hasForeColor() const
{
    return this->_forecolor.isValid();
}

bool QHexMetadata::hasComment() const
{
    return !this->_comment.isEmpty();
}

const QColor& QHexMetadata::backColor() const
{
    return this->_backcolor;
}

const QColor &QHexMetadata::foreColor() const
{
    return this->_forecolor;
}

const QString &QHexMetadata::comment() const
{
    return this->_comment;
}

void QHexMetadata::setBackColor(const QColor &c)
{
    this->_backcolor = c;
}

void QHexMetadata::setForeColor(const QColor &c)
{
    this->_forecolor = c;
}

void QHexMetadata::setComment(const QString &s)
{
    this->_comment = s;
}

void QHexMetadata::clearComment()
{
    this->_comment.clear();
}

void QHexMetadata::clearColors()
{
    this->_backcolor = QColor();
    this->_forecolor = QColor();
}
