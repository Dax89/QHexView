#include "qhexdocument.h"
#include "commands/insertcommand.h"
#include "commands/removecommand.h"
#include "commands/replacecommand.h"
#include <functional>
#include <QGuiApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QBuffer>
#include <QFile>

static void removeMetadata(MetadataMultiHash& metadata, std::function<bool(QHexMetadata*)> cb)
{
    MetadataHashIterator it(metadata);

    while(it.hasNext())
    {
        sinteger_t i = 0;
        MetadataList values = metadata.values(it.key());

        while(i < values.length())
        {
            if(cb(values[i]))
                values.removeAt(i);
            else
                i++;
        }
    }
}

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

const MetadataMultiHash &QHexDocument::metadata() const
{
    return this->_metadata;
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

QString QHexDocument::commentString(integer_t offset) const
{
    if(!this->_comments.contains(offset))
        return QString();

    return this->_comments[offset]->comment();
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

void QHexDocument::highlightFore(integer_t startoffset, integer_t endoffset, const QColor &c)
{
    if((startoffset == endoffset) || !c.isValid())
        return;

    QHexMetadata* metadata = new QHexMetadata(startoffset, endoffset, this);
    metadata->setForeColor(c);
    this->_metadata.insert(startoffset, metadata);

    if(!this->_bulkmetadata)
        emit documentChanged();
}

void QHexDocument::highlightBack(integer_t startoffset, integer_t endoffset, const QColor &c)
{
    if((startoffset == endoffset) || !c.isValid())
        return;

    QHexMetadata* metadata = new QHexMetadata(startoffset, endoffset, this);
    metadata->setBackColor(c);
    this->_metadata.insert(startoffset, metadata);

    if(!this->_bulkmetadata)
        emit documentChanged();
}

void QHexDocument::highlightForeRange(integer_t offset, integer_t length, const QColor &c)
{
    this->highlightFore(offset, offset + length - 1, c);
}

void QHexDocument::highlightBackRange(integer_t offset, integer_t length, const QColor &c)
{
    this->highlightBack(offset, offset + length - 1, c);
}

void QHexDocument::comment(integer_t startoffset, integer_t endoffset, const QString &s)
{
    if((startoffset == endoffset) || s.isEmpty())
        return;

    QHexMetadata* metadata = new QHexMetadata(startoffset, endoffset, this);
    metadata->setComment(s);
    this->_metadata.insert(startoffset, metadata);

    for(integer_t i = startoffset; i <= endoffset; i++) // NOTE: Find a better algorithm
        this->_comments[i] = metadata;

    if(!this->_bulkmetadata)
        emit documentChanged();
}

void QHexDocument::commentRange(integer_t offset, integer_t length, const QString &s)
{
    this->comment(offset, offset + length - 1, s);
}

void QHexDocument::clearHighlighting()
{
    if(this->_metadata.isEmpty())
        return;

    removeMetadata(this->_metadata, [](QHexMetadata* metadata) -> bool {
            if(metadata->hasComment()) {
                metadata->clearColors();
                return false;
            }

            return true;
    });

    emit documentChanged();
}

void QHexDocument::clearComments()
{
    if(this->_metadata.isEmpty())
        return;

    removeMetadata(this->_metadata, [this](QHexMetadata* metadata) -> bool {
            bool doremove = false;

            if(metadata->hasForeColor() || metadata->hasBackColor())
                metadata->clearComment();
            else
                doremove = true;

            for(integer_t i = metadata->startOffset(); i <= metadata->endOffset(); i++) // NOTE: Find a better algorithm
                this->_comments.remove(i);

            return doremove;
    });

    emit documentChanged();
}

void QHexDocument::beginMetadata()
{
    this->_bulkmetadata = true;
}

void QHexDocument::endMetadata()
{
    this->_bulkmetadata = false;
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
