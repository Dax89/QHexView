#ifndef QHEXMETADATA_H
#define QHEXMETADATA_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QMap>
#include "qhexmetadataitem.h"

typedef QList<QHexMetadataItem*> MetadataList;
typedef QHash<integer_t, MetadataList> MetadataMap;

class QHexMetadata : public QObject
{
    Q_OBJECT

    private:
        typedef QMap<integer_t, integer_t> RangeMap;
        typedef QPair<RangeMap::const_iterator, RangeMap::const_iterator> RangeIterators;

    public:
        explicit QHexMetadata(QObject *parent = 0);
        void insert(QHexMetadataItem* metaitem);
        void beginMetadata();
        void endMetadata();
        void clearHighlighting();
        void clearComments();
        MetadataList fromOffset(integer_t offset) const;
        QString commentString(integer_t offset) const;

    private:
        const QHexMetadataItem *comment(integer_t offset) const;

    signals:
        void metadataChanged();

    private:
        RangeMap _ranges;
        RangeIterators _lastrange;
        MetadataMap _metadata;
        bool _bulkmetadata;
};

#endif // QHEXMETADATA_H
