#ifndef QHEXMETADATA_H
#define QHEXMETADATA_H

#include <QMultiHash>
#include <QObject>
#include <QColor>
#include "gapbuffer.h"

class QHexMetadata : public QObject
{
    Q_OBJECT

    public:
        explicit QHexMetadata(integer_t startoffset, integer_t endoffset, QObject *parent = 0);
        integer_t startOffset() const;
        integer_t endOffset() const;
        bool contains(integer_t offset) const;
        bool hasBackColor() const;
        bool hasForeColor() const;
        bool hasComment() const;
        const QColor& backColor() const;
        const QColor& foreColor() const;
        const QString& comment() const;
        void setBackColor(const QColor& c);
        void setForeColor(const QColor& c);
        void setComment(const QString& s);
        void clearComment();
        void clearColors();

    private:
        integer_t _startoffset, _endoffset;
        QColor _backcolor, _forecolor;
        QString _comment;
};

typedef QMultiHash<integer_t, QHexMetadata*> MetadataMultiHash;
typedef QMutableHashIterator<integer_t, QHexMetadata*> MetadataHashIterator;
typedef QList<QHexMetadata*> MetadataList;

#endif // QHEXMETADATA_H
