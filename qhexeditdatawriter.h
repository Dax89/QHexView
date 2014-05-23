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
        void writeUInt8(qint64 pos, char d);
        void writeUInt16(qint64 pos, quint16 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        void writeUInt32(qint64 pos, quint32 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        void writeUInt64(qint64 pos, quint64 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        void writeInt8(qint64 pos, uchar d);
        void writeInt16(qint64 pos, qint16 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        void writeInt32(qint64 pos, qint32 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);
        void writeInt64(qint64 pos, qint64 d, QSysInfo::Endian endian = QSysInfo::ByteOrder);

    private:
        template<typename T> void writeType(qint64 pos, T d, QSysInfo::Endian endian)
        {
            QByteArray ba;
            QDataStream ds(&ba, QIODevice::WriteOnly);

            if(endian == QSysInfo::LittleEndian)
                ds.setByteOrder(QDataStream::LittleEndian);
            else
                ds.setByteOrder(QDataStream::BigEndian);

            ds << d;
            this->replace(pos, ba);
        }

    private:
        QHexEditData* _hexeditdata;
};

#endif // QHEXEDITDATAWRITER_H
