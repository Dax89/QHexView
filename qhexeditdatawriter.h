#ifndef QHEXEDITDATAWRITER_H
#define QHEXEDITDATAWRITER_H

#include <QtCore>
#include "qhexeditdata.h"

class QHexEditDataWriter : public QObject
{
    Q_OBJECT

    public:
        explicit QHexEditDataWriter(QHexEditData* hexeditdata, QObject* parent = 0);
        const QHexEditData* hexEditData() const;
        void append(const QByteArray& ba);
        void insert(qint64 pos, uchar ch);
        void insert(qint64 pos, const QByteArray& ba);
        void remove(qint64 pos, qint64 len);
        void replace(qint64 pos, uchar b);
        void replace(qint64 pos, qint64 len, uchar b);
        void replace(qint64 pos, const QByteArray& ba);
        void replace(qint64 pos, qint64 len, const QByteArray& ba);

    private:
        QHexEditData* _hexeditdata;
};

#endif // QHEXEDITDATAWRITER_H
