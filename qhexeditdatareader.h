#ifndef QHEXEDITDATAREADER_H
#define QHEXEDITDATAREADER_H

#include <QtCore>
#include "qhexeditdata.h"

class QHexEditDataReader : public QObject
{
    Q_OBJECT

    public:
        explicit QHexEditDataReader(QHexEditData* hexeditdata, QObject* parent = 0);
        const QHexEditData* hexEditData() const;
        uchar at(qint64 pos);
        qint64 indexOf(const QByteArray& ba, qint64 start);
        QByteArray read(qint64 pos, qint64 len);
        QString readString(qint64 pos, qint64 maxlen = -1);
        quint16 readUInt16(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        quint32 readUInt32(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        quint64 readUInt64(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        qint16 readInt16(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        qint32 readInt32(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        qint64 readInt64(qint64 pos, QSysInfo::Endian endian = QSysInfo::ByteOrder);

    private slots:
        void onDataChanged(qint64 offset, qint64, QHexEditData::ActionType);

    private:
        void bufferizeData(qint64 pos);
        bool inBuffer(qint64 pos);
        bool needsBuffering(qint64 pos);

    private:
        bool _dirtybuffer;
        qint64 _bufferpos;
        QByteArray _buffereddata;
        QHexEditData* _hexeditdata;
};

#endif // QHEXEDITDATAREADER_H
