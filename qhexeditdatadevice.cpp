#include "qhexeditdatadevice.h"

QHexEditDataDevice::QHexEditDataDevice(QHexEditData *hexeditdata): QIODevice(hexeditdata), _hexeditdata(hexeditdata)
{
    this->_reader = new QHexEditDataReader(hexeditdata, this);
    this->_writer = new QHexEditDataWriter(hexeditdata, this);
}

qint64 QHexEditDataDevice::size()
{
    return this->_hexeditdata->length();
}

qint64 QHexEditDataDevice::readData(char *data, qint64 maxlen)
{
    if(!this->_hexeditdata)
        return 0;

    QByteArray ba = this->_reader->read(this->pos(), maxlen);

    if(!ba.isEmpty())
    {
        memcpy(data, ba.data(), ba.length());
        return ba.length();
    }

    return -1;
}

qint64 QHexEditDataDevice::writeData(const char *data, qint64 len)
{
    if(!this->_hexeditdata)
        return 0;

    this->_writer->replace(this->pos(), QByteArray::fromRawData(data, len));
    return len; /* Append if needed */
}
