#include "qhexeditdata.h"

const qint64 QHexEditData::BUFFER_SIZE = 8192;

QHexEditData::QHexEditData(QIODevice *iodevice, QObject *parent): QObject(parent), _iodevice(iodevice), _buffereddatapos(-1), _length(iodevice->size()), _lastpos(-1), _lastaction(QHexEditData::None)
{
    this->_modlist.append(new ModifiedItem(0, iodevice->size(), false));
}

QUndoStack *QHexEditData::undoStack()
{
    return &this->_undostack;
}

uchar QHexEditData::at(qint64 pos)
{
    if(pos < 0)
        pos = 0;
    else if(pos >= this->_length)
        pos = this->_length - 1;

    int i;
    qint64 modoffset;
    ModifiedItem* mi = this->modifiedItem(pos, &modoffset, &i);

    uchar ch;
    qint64 mioffset = pos - modoffset;

    if(mi->modified())
        return this->_modbuffer.at(mi->pos() +  mioffset);
    else
    {
        if(this->needsBuffering(pos))
            this->bufferizeData(pos);

        return this->_buffereddata.at(pos - this->_buffereddatapos);
    }

    return ch;
}

qint64 QHexEditData::indexOf(const QByteArray &ba, qint64 start)
{
    if(!ba.length())
        return -1;

    for(qint64 pos = start; pos < this->length(); pos++)
    {
        QByteArray cba = this->read(pos, ba.length());

        if(cba == ba)
            return pos;
    }

    return -1;
}

void QHexEditData::insert(qint64 pos, uchar ch)
{
    this->insert(pos, QByteArray().append(ch));
}

void QHexEditData::insert(qint64 pos, const QByteArray &ba)
{
    InsertCommand* ic = this->internalInsert(pos, ba, QHexEditData::Insert);

    if(ic)
    {
        this->recordAction(QHexEditData::Insert, pos + ba.length());
        this->_undostack.push(ic);
    }
}

void QHexEditData::remove(qint64 pos, qint64 len)
{
    RemoveCommand* rc = this->internalRemove(pos, len, QHexEditData::Remove);

    if(rc)
    {
        this->recordAction(QHexEditData::Remove, pos);
        this->_undostack.push(rc);
    }
}

void QHexEditData::replace(qint64 pos, uchar b)
{
    this->replace(pos, 1, b);
}

void QHexEditData::replace(qint64 pos, qint64 len, uchar b)
{
    this->replace(pos, len, QByteArray().append(b));
}

void QHexEditData::replace(qint64 pos, const QByteArray &ba)
{
    this->replace(pos, ba.length(), ba);
}

void QHexEditData::replace(qint64 pos, qint64 len, const QByteArray &ba)
{
    if(pos > this->length() || ba.isEmpty() || !this->modifiedItem(pos))
        return;

    this->_undostack.push(new ReplaceCommand(pos, len, ba, this));
    this->recordAction(QHexEditData::Replace, pos + ba.length());
}

QByteArray QHexEditData::read(qint64 pos, qint64 len)
{
    if(len > this->_length)
        len = this->_length;

    qint64 modoffset;
    ModifiedItem* mi = this->modifiedItem(pos, &modoffset);

    if(!mi)
        return QByteArray();

    QByteArray resba(len, 0x00);
    qint64 copied = 0, mioffset = pos - modoffset;

    while(len > 0)
    {
        qint64 copylen = qMin(mi->length() - mioffset, len);

        if(mi->modified())
            memcpy(resba.data() + copied, this->_modbuffer.constData() + (mi->pos() + mioffset), copylen);
        else
        {
            qint64 currpos = pos + copied;

            if(this->needsBuffering(currpos))
                this->bufferizeData(currpos);

            qint64 bufferedpos = currpos - this->_buffereddatapos;
            copylen = qMin(copylen, QHexEditData::BUFFER_SIZE);
            memcpy(resba.data() + copied, this->_buffereddata.constData() + bufferedpos, copylen);
        }

        copied += copylen;
        len -= copylen;
        mioffset = 0;

        if(len > 0)
            mi = this->modifiedItem(pos, &modoffset);
    }

    return resba;
}

qint64 QHexEditData::length() const
{
    return this->_length;
}

bool QHexEditData::save()
{
    return this->saveTo(this->_iodevice);
}

bool QHexEditData::saveTo(QIODevice *iodevice)
{
    if(!iodevice->isOpen())
        iodevice->open(QIODevice::WriteOnly);

    if(!iodevice->isWritable())
        return false;

    qint64 outseekpos = 0;
    this->_iodevice->reset();

    for(int i = 0; i < this->_modlist.length(); i++)
    {
        ModifiedItem* mi = this->_modlist[i];
        iodevice->seek(outseekpos);

        if(mi->modified())
            iodevice->write(this->_modbuffer.mid(mi->pos(), mi->length()));
        else if(iodevice != this->_iodevice)
        {
            this->_iodevice->seek(mi->pos());
            QByteArray ba = this->_iodevice->read(mi->length());
            iodevice->write(ba);
        }

        outseekpos += mi->length();
    }

    return true;
}

QHexEditData *QHexEditData::fromDevice(QIODevice *iodevice)
{
    if(!iodevice->isOpen())
        iodevice->open(QFile::ReadWrite);

    return new QHexEditData(iodevice);
}

QHexEditData *QHexEditData::fromFile(QString filename)
{
    QFile* f = new QFile(filename);
    f->open(QFile::ReadWrite);

    return QHexEditData::fromDevice(f);
}

QHexEditData *QHexEditData::fromMemory(const QByteArray &ba)
{
    QBuffer* b = new QBuffer();
    b->setData(ba);
    b->open(QFile::ReadOnly);

    return QHexEditData::fromDevice(b);
}

QHexEditData::InsertCommand* QHexEditData::internalInsert(qint64 pos, const QByteArray &ba, QHexEditData::ActionType act)
{
    if(pos < 0 || pos > this->length() || !ba.length())
        return nullptr;

    int i;
    qint64 datapos;
    ModifiedItem* mi;

    if(!this->_modlist.isEmpty())
    {
        mi = this->modifiedItem(pos, &datapos, &i);

        if(!mi)
            return nullptr;
    }
    else
    {
        pos = 0;
        datapos = 0;
    }

    bool optimize = false;
    ModifyList oldml, newml;
    qint64 modoffset = pos - datapos;
    qint64 buffoffset = this->updateBuffer(ba);

    if(!modoffset)
    {
        optimize = (act == QHexEditData::Insert) && this->canOptimize(act, pos);
        newml.append(new ModifiedItem(buffoffset, ba.length()));
    }
    else
    {
        oldml.append(mi);
        newml.append(new ModifiedItem(mi->pos(), modoffset, mi->modified()));
        newml.append(new ModifiedItem(buffoffset, ba.length()));
        newml.append(new ModifiedItem(mi->pos() + modoffset, mi->length() - modoffset, mi->modified()));
    }

    this->_length += ba.length();
    return new InsertCommand(i, pos, oldml, newml, this, optimize);
}

QHexEditData::RemoveCommand* QHexEditData::internalRemove(qint64 pos, qint64 len, ActionType act)
{
    if(pos < 0 || !len || pos >= this->length() || pos > (this->length() - len))
        return nullptr;

    int i;
    qint64 datapos;
    ModifiedItem* mi = this->modifiedItem(pos, &datapos, &i);

    if(!mi)
        return nullptr;

    QList<ModifiedItem*> olditems, newitems;
    qint64 remoffset = pos - datapos, remlength = len;

    if(remoffset)
    {
        newitems.append(new ModifiedItem(mi->pos(), remoffset, mi->modified()));

        if((remoffset + remlength) < mi->length())
            newitems.append(new ModifiedItem(mi->pos() + remoffset + remlength, mi->length() - remoffset - remlength, mi->modified()));

        remlength -= qMin(remlength, mi->length() - remoffset);
        olditems.append(mi);
        i++;
    }

    while(remlength && (i < this->_modlist.length()))
    {
        mi = this->_modlist[i];

        if(remlength < mi->length())
            newitems.append(new ModifiedItem(mi->pos() + remlength, mi->length() - remlength, mi->modified()));

        remlength -= qMin(remlength, mi->length());
        olditems.append(mi);
        i++;
    }

    this->_length -= len;
    return new RemoveCommand(i - olditems.length(), pos, olditems, newitems, this);
}

bool QHexEditData::canOptimize(QHexEditData::ActionType at, qint64 pos)
{
    return (this->_lastaction == at) && (this->_lastpos == pos);
}

void QHexEditData::recordAction(QHexEditData::ActionType at, qint64 pos)
{
    this->_lastpos = pos;
    this->_lastaction = at;
}

qint64 QHexEditData::updateBuffer(const QByteArray &ba)
{
    qint64 pos = this->_modbuffer.length();
    this->_modbuffer.append(ba);
    return pos;
}

void QHexEditData::bufferizeData(qint64 pos)
{
    qint64 s = this->_iodevice->size();

    if(s < QHexEditData::BUFFER_SIZE) /* Simple Case: Data Small than BUFFER_SIZE */
    {
        this->_buffereddatapos = 0;
        this->_buffereddata = this->_iodevice->readAll();
        return;
    }

    if((s - pos) < QHexEditData::BUFFER_SIZE)
        pos = s - QHexEditData::BUFFER_SIZE;

    this->_buffereddatapos = pos;

    this->_iodevice->seek(pos);
    this->_buffereddata = this->_iodevice->read(qMin(s, QHexEditData::BUFFER_SIZE));
}

bool QHexEditData::needsBuffering(qint64 pos)
{
    return (this->_buffereddatapos == -1) || (pos < this->_buffereddatapos) || (pos >= (this->_buffereddatapos + QHexEditData::BUFFER_SIZE));
}

QHexEditData::ModifiedItem* QHexEditData::modifiedItem(qint64 pos, qint64* datapos, int *index)
{
    int i = -1;
    qint64 currpos = 0;
    ModifiedItem* mi = nullptr;

    for(i = 0; i < this->_modlist.length(); i++)
    {
        mi = this->_modlist[i];

        if((pos >= currpos) && (pos < (currpos + mi->length())))
        {
            if(datapos)
                *datapos = currpos;

            if(index)
                *index = i;

            return mi;
        }

        currpos += mi->length();
    }

    if(mi && (pos == currpos)) /* Return Last Item for Append */
    {
        if(datapos)
            *datapos = currpos;

        if(index)
            *index = i;

        return mi;
    }

    return nullptr;
}


void QHexEditData::append(const QByteArray &ba)
{
    this->insert(this->length(), ba);
}
