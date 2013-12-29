#include "qhexeditdata.h"

QHexEditData::QHexEditData(QIODevice *iodevice, QObject *parent): QObject(parent)
{
    this->_iodevice = iodevice;
    this->_iodevice->open(QIODevice::ReadOnly);

    this->_bytes = this->_iodevice->readAll(); /* FIXME: !!! LARGE FILE SUPPORT !!! */
}

QHexEditData::~QHexEditData()
{
    if(this->_iodevice)
        this->_iodevice->close();
}

const char *QHexEditData::constDataPtr()
{
    return this->_bytes.constData();
}

char *QHexEditData::dataPtr()
{
    return this->_bytes.data();
}

uchar QHexEditData::at(qint64 i)
{
    return this->_bytes.at(i);
}

qint64 QHexEditData::indexOf(const QByteArray &ba, qint64 start)
{
    return static_cast<qint64>(this->_bytes.indexOf(ba, start));
}

QByteArray QHexEditData::read(qint64 pos, qint64 len)
{
    return this->_bytes.mid(pos, len);
}

void QHexEditData::write(const QByteArray &ba)
{
    this->insert(this->_bytes.size(), ba);
}

qint64 QHexEditData::length()
{
    return this->_bytes.length();
}

void QHexEditData::save(QString filename)
{
    QFile f(filename);
    f.open(QFile::WriteOnly);
    f.write(this->_bytes);
    f.close();
}

void QHexEditData::insert(qint64 pos, const QByteArray &newba)
{
    if(newba.length())
    {
        this->_bytes.insert(pos, newba);
        emit dataChanged(pos, newba.length());
    }
}

void QHexEditData::replace(qint64 pos, qint64 len, const QByteArray &newba)
{
    if(newba.length())
    {
        this->_bytes.replace(pos, len, newba);
        emit dataChanged(pos, newba.length());
    }
}

void QHexEditData::remove(qint64 pos, qint64 len)
{
    if(len > 0)
    {
        this->_bytes.remove(pos, len);
        emit dataChanged(pos, len);
    }
}
