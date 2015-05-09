#include "qhexeditdata.h"

const qint64 QHexEditData::BUFFER_SIZE = 8192;

QHexEditData::QHexEditData(QIODevice *iodevice, QObject *parent): QObject(parent), _iodevice(iodevice), _length(iodevice->size()), _devicelength(iodevice->size()), _lastpos(-1), _lastaction(QHexEditData::None)
{
    qRegisterMetaType<QHexEditData::ActionType>("QHexEditData::ActionType");
    this->_modlist.append(new ModifiedItem(0, iodevice->size(), false));
}

QHexEditData::~QHexEditData()
{
    foreach(ModifiedItem* mi, this->_modlist)
        delete mi;

    this->_modlist.clear();

    if(this->_iodevice->isOpen())
        this->_iodevice->close();
}

QUndoStack *QHexEditData::undoStack()
{
    return &this->_undostack;
}

qint64 QHexEditData::length() const
{
    return this->_length;
}

bool QHexEditData::isReadOnly() const
{
    return !this->_iodevice->isWritable();
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
    QFileInfo fi(filename);
    QFile* f = new QFile(filename);

    if(fi.isWritable())
        f->open(QFile::ReadWrite);
    else
        f->open(QFile::ReadOnly);

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
        return NULL;

    int i;
    qint64 datapos;
    ModifiedItem* mi;

    if(!this->_modlist.isEmpty())
    {
        mi = this->modifiedItem(pos, &datapos, &i);

        if(!mi)
            return NULL;
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
    emit dataChanged(pos, ba.length(), act);
    return new InsertCommand(i, pos, oldml, newml, this, optimize);
}

QHexEditData::RemoveCommand* QHexEditData::internalRemove(qint64 pos, qint64 len, QHexEditData::ActionType act)
{
    if(pos < 0 || !len || pos >= this->length() || pos > (this->length() - len))
        return NULL;

    int i;
    qint64 datapos;
    ModifiedItem* mi = this->modifiedItem(pos, &datapos, &i);

    if(!mi)
        return NULL;

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
    emit dataChanged(pos, len, act);
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
    ModifiedItem* mi = NULL;

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

    return NULL;
}
