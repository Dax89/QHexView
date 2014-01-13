#include "qhexeditdata.h"

QHexEditData::QHexEditData(QIODevice *iodevice, QObject *parent): QObject(parent), _iodevice(iodevice), _length(iodevice->size()), _lastpos(-1), _lastaction(QHexEditData::None)
{
    this->_modlist.append(new ModifiedItem(0, iodevice->size(), false));
}

QUndoStack *QHexEditData::undoStack()
{
    return &this->_undostack;
}

uchar QHexEditData::at(qint64 i)
{
    return this->read(i, 1)[0];
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
    int i;
    QByteArray resba;
    qint64 modoffset;

    ModifiedItem* mi = this->modifiedItem(pos, &modoffset, &i);

    if(!mi)
        return QByteArray();

    qint64 currpos = pos, mioffset = pos - modoffset;

    while(len && (i < this->_modlist.length()))
    {
        QByteArray ba;
        mi = this->_modlist[i];
        qint64 copylen = qMin(mi->length() - mioffset, len);

        if(mi->modified())
            ba = this->_modbuffer.mid(mi->pos() + mioffset, copylen);
        else
        {
            this->_iodevice->seek(mi->pos() + mioffset);
            ba = this->_iodevice->read(copylen);
        }

        resba.append(ba);

        currpos += copylen;
        len -= copylen;
        mioffset = 0;
        i++;
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
    ModifiedItem* mi = this->modifiedItem(pos, &datapos, &i);

    if(!mi)
        return nullptr;

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
