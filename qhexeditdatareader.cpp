#include "qhexeditdatareader.h"

QHexEditDataReader::QHexEditDataReader(QHexEditData *hexeditdata, QObject *parent): QObject(parent), _dirtybuffer(false), _bufferpos(-1), _hexeditdata(hexeditdata)
{
    connect(hexeditdata, SIGNAL(dataChanged(qint64,qint64,QHexEditData::ActionType)), this, SLOT(onDataChanged(qint64,qint64,QHexEditData::ActionType)));
    this->_buffereddata.resize(QHexEditData::BUFFER_SIZE);
}

const QHexEditData *QHexEditDataReader::hexEditData() const
{
    return this->_hexeditdata;
}

uchar QHexEditDataReader::at(qint64 pos)
{
    if(pos < 0)
        pos = 0;
    else if(pos >= this->_hexeditdata->_length)
        pos = this->_hexeditdata->_length - 1;

    if(this->needsBuffering(pos))
        this->bufferizeData(pos);

    return static_cast<uchar>(this->_buffereddata.at(pos - this->_bufferpos));
}

qint64 QHexEditDataReader::indexOf(const QByteArray &ba, qint64 start)
{
    if(!ba.length())
        return -1;

    for(qint64 pos = start; pos < this->_hexeditdata->_length; pos++)
    {
        QByteArray cba = this->read(pos, ba.length());

        if(cba == ba)
            return pos;
    }

    return -1;
}

QByteArray QHexEditDataReader::read(qint64 pos, qint64 len)
{
    if(pos >= this->_hexeditdata->_length)
        return QByteArray();
    else if(len > this->_hexeditdata->_length)
        len = this->_hexeditdata->_length;

    QByteArray resba(len, Qt::Uninitialized);
    qint64 currpos = pos, copied = 0;

    while(len > 0)
    {
        if(this->needsBuffering(currpos))
            this->bufferizeData(currpos);

        qint64 bufferedpos = currpos - this->_bufferpos, copylen = qMin(len, QHexEditData::BUFFER_SIZE);
        memcpy(resba.data() + copied, this->_buffereddata.constData() + bufferedpos, copylen);

        copied += copylen;
        currpos += copylen;
        len -= copylen;
    }

    return resba;
}

QString QHexEditDataReader::readString(qint64 pos, qint64 maxlen)
{
    QString s;

    if(maxlen == -1)
        maxlen = this->_hexeditdata->length();
    else
        maxlen = qMin(this->_hexeditdata->length(), maxlen);

    for(qint64 i = 0; i < maxlen; i++)
    {
        if((pos + i) >= this->_hexeditdata->length())
            break;

        QChar ch(this->at(pos + i));

        if(!ch.unicode() || ch.unicode() >= 128)
            break;

        s.append(ch);
    }

    return s;
}

quint16 QHexEditDataReader::readUInt16(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 2);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    quint16 val;
    ds >> val;
    return val;
}

quint32 QHexEditDataReader::readUInt32(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 4);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    quint32 val;
    ds >> val;
    return val;
}

quint64 QHexEditDataReader::readUInt64(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 8);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    quint64 val;
    ds >> val;
    return val;
}

qint16 QHexEditDataReader::readInt16(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 2);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    qint16 val;
    ds >> val;
    return val;
}

qint32 QHexEditDataReader::readInt32(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 4);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    qint32 val;
    ds >> val;
    return val;
}

qint64 QHexEditDataReader::readInt64(qint64 pos, QSysInfo::Endian endian)
{
    QByteArray ba = this->read(pos, 8);
    QDataStream ds(ba);

    if(endian == QSysInfo::LittleEndian)
        ds.setByteOrder(QDataStream::LittleEndian);
    else
        ds.setByteOrder(QDataStream::BigEndian);

    qint64 val;
    ds >> val;
    return val;
}

void QHexEditDataReader::onDataChanged(qint64 pos, qint64, QHexEditData::ActionType)
{
    if(this->inBuffer(pos))
        this->_dirtybuffer = true;
}

bool QHexEditDataReader::needsBuffering(qint64 pos)
{
    return this->_dirtybuffer || (this->_bufferpos == -1) || (pos < this->_bufferpos) || (pos >= (this->_bufferpos + QHexEditData::BUFFER_SIZE));
}

void QHexEditDataReader::bufferizeData(qint64 pos)
{
    // Adjust pos in order to fit in buffer (if the loaded data cannot be buffer entirely: datasize > 8KB)
    if((this->_hexeditdata->_length > QHexEditData::BUFFER_SIZE) && (pos + QHexEditData::BUFFER_SIZE) >= this->_hexeditdata->_length)
        pos = this->_hexeditdata->_length - QHexEditData::BUFFER_SIZE;

    this->_bufferpos = pos;

    int i = -1;
    qint64 currpos = pos, mipos = 0, copied = 0;

    if(!this->_hexeditdata->modifiedItem(pos, &mipos, &i))
        return;

    while((copied < QHexEditData::BUFFER_SIZE) && (i < this->_hexeditdata->_modlist.length()))
    {
        QHexEditData::ModifiedItem* mi = this->_hexeditdata->_modlist[i];
        qint64 bufferoffset = currpos - mipos, copylen = qMin(mi->length() - bufferoffset, QHexEditData::BUFFER_SIZE - copied);

        if(mi->modified())
            memcpy(this->_buffereddata.data() + copied, this->_hexeditdata->_modbuffer.data() + bufferoffset + mi->pos(), copylen);
        else
        {
            this->_hexeditdata->_mutex.lock();
            this->_hexeditdata->_iodevice->seek(bufferoffset + mi->pos());
            this->_hexeditdata->_iodevice->read(this->_buffereddata.data() + copied, copylen);
            this->_hexeditdata->_mutex.unlock();
        }

        mipos += mi->length();
        currpos += copylen;
        copied += copylen;
        i++;
    }

    this->_dirtybuffer = false;
}

bool QHexEditDataReader::inBuffer(qint64 pos)
{
    return (pos >= this->_bufferpos) && (pos < (this->_bufferpos + QHexEditData::BUFFER_SIZE));
}
