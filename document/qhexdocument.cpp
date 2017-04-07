#include "qhexdocument.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QBuffer>
#include <QFile>
#include "commands/insertcommand.h"
#include "commands/removecommand.h"
#include "commands/replacecommand.h"

QHexDocument::QHexDocument(QIODevice *device, QObject *parent): QObject(parent)
{
    this->_baseaddress = 0;
    this->_gapbuffer = new GapBuffer(device);
    this->_cursor = new QHexCursor(this);

    connect(&this->_undostack, &QUndoStack::canUndoChanged, this, &QHexDocument::canUndoChanged);
    connect(&this->_undostack, &QUndoStack::canRedoChanged, this, &QHexDocument::canRedoChanged);
}

QHexDocument::~QHexDocument()
{
    delete this->_gapbuffer;
    this->_gapbuffer = NULL;
}

QHexCursor *QHexDocument::cursor() const
{
    return this->_cursor;
}

integer_t QHexDocument::length() const
{
    return this->_gapbuffer->length();
}

integer_t QHexDocument::baseAddress() const
{
    return this->_baseaddress;
}

bool QHexDocument::canUndo() const
{
    return this->_undostack.canUndo();
}

bool QHexDocument::canRedo() const
{
    return this->_undostack.canRedo();
}

QByteArray QHexDocument::read(integer_t offset, integer_t len)
{
    return this->_gapbuffer->read(offset, len);
}

QByteArray QHexDocument::selectedBytes() const
{
    if(!this->_cursor->hasSelection())
        return QByteArray();

    return this->_gapbuffer->read(this->_cursor->selectionStart(), this->_cursor->selectionEnd());
}

char QHexDocument::at(integer_t offset) const
{
    return this->_gapbuffer->at(offset);
}

void QHexDocument::setBaseAddress(integer_t baseaddress)
{
    if(this->_baseaddress == baseaddress)
        return;

    this->_baseaddress = baseaddress;
    emit baseAddressChanged();
}

QHexDocument *QHexDocument::fromDevice(QIODevice *iodevice)
{
    if(!iodevice->isOpen())
        iodevice->open(QFile::ReadWrite);

    return new QHexDocument(iodevice);
}

QHexDocument *QHexDocument::fromFile(QString filename)
{
    QFileInfo fi(filename);
    QFile* f = new QFile(filename);

    if(fi.isWritable())
        f->open(QFile::ReadWrite);
    else
        f->open(QFile::ReadOnly);

    QHexDocument* hexeditdata = QHexDocument::fromDevice(f);
    f->setParent(hexeditdata);
    return hexeditdata;
}

QHexDocument *QHexDocument::fromMemory(const QByteArray &ba)
{
    QBuffer* b = new QBuffer();
    b->setData(ba);
    b->open(QFile::ReadOnly);

    QHexDocument* hexeditdata = QHexDocument::fromDevice(b);
    b->setParent(hexeditdata);
    return hexeditdata;
}

void QHexDocument::undo()
{
    this->_undostack.undo();
    emit documentChanged();
}

void QHexDocument::redo()
{
    this->_undostack.redo();
    emit documentChanged();
}

void QHexDocument::cut()
{
    if(!this->_cursor->hasSelection())
        return;

    QClipboard* c = qApp->clipboard();
    c->setText(QString(this->selectedBytes()));
    this->_cursor->removeSelection();
}

void QHexDocument::copy()
{
    if(!this->_cursor->hasSelection())
        return;

    QClipboard* c = qApp->clipboard();
    c->setText(QString(this->selectedBytes()));
    this->_cursor->removeSelection();
}

void QHexDocument::paste()
{
    this->_cursor->removeSelection();

    QClipboard* c = qApp->clipboard();
    QByteArray data = c->text().toUtf8();

    this->insert(this->_cursor->offset(), data);
    this->_cursor->moveOffset(data.length());
}

void QHexDocument::insert(integer_t offset, uchar b)
{
    this->insert(offset, QByteArray(1, b));
}

void QHexDocument::replace(integer_t offset, uchar b)
{
    this->replace(offset, QByteArray(1, b));
}

void QHexDocument::insert(integer_t offset, const QByteArray &data)
{
    this->_undostack.push(new InsertCommand(this->_gapbuffer, offset, data));
    emit documentChanged();
}

void QHexDocument::replace(integer_t offset, const QByteArray &data)
{
    this->_undostack.push(new ReplaceCommand(this->_gapbuffer, offset, data));
    emit documentChanged();
}

void QHexDocument::remove(integer_t offset, integer_t len)
{
    this->_undostack.push(new RemoveCommand(this->_gapbuffer, offset, len));
    emit documentChanged();
}

QByteArray QHexDocument::read(integer_t offset, integer_t len) const
{
    return this->_gapbuffer->read(offset, len);
}

bool QHexDocument::saveTo(QIODevice *device)
{
    if(!device->isWritable())
        return false;

    device->write(this->_gapbuffer->toByteArray());
    return true;
}

bool QHexDocument::isEmpty() const
{
    return this->_gapbuffer->length() <= 0;
}
