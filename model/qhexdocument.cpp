#include "qhexdocument.h"
#include "model/buffer/qmemorybuffer.h"
#include "commands/insertcommand.h"
#include "commands/removecommand.h"
#include "commands/replacecommand.h"
#include "../qhexview.h"
#include "qhexcursor.h"
#include <QApplication>
#include <QClipboard>
#include <QBuffer>
#include <QPalette>
#include <QFile>
#include <memory>
#include <cmath>

QHexDocument::QHexDocument(QHexBuffer *buffer, const QHexOptions& options, QObject* parent): QObject(parent), m_options(options), m_baseaddress(0)
{
    m_hexmetadata = new QHexMetadata(&m_options, this);
    m_hexcursor = new QHexCursor(this);

    m_buffer = buffer;
    m_buffer->setParent(this); // Take Ownership

    connect(&m_undostack, &QUndoStack::canUndoChanged, this, &QHexDocument::canUndoChanged);
    connect(&m_undostack, &QUndoStack::canRedoChanged, this, &QHexDocument::canRedoChanged);
}

void QHexDocument::checkOptions(QPalette palette)
{
    if(m_options.grouplength > m_options.linelength) m_options.grouplength = m_options.linelength;

    // Round to nearest multiple of 2
    m_options.grouplength = 1u << (static_cast<unsigned int>(std::floor(m_options.grouplength / 2.0)));

    if(m_options.grouplength <= 1) m_options.grouplength = 1;

    if(!m_options.headercolor.isValid())
        m_options.headercolor = palette.color(QPalette::Normal, QPalette::Highlight);
}

qint64 QHexDocument::lastColumn() const { return this->getLine(this->lastLine()).size(); }
qint64 QHexDocument::lastLine() const { return this->lines() - 1; }

quint64 QHexDocument::lines() const
{
    if(!m_buffer) return 0;

    auto lines = static_cast<quint64>(std::ceil(m_buffer->length() / m_options.linelength));
    return !m_buffer->isEmpty() && !lines ? 1 : lines;
}

bool QHexDocument::isEmpty() const { return m_buffer->isEmpty(); }
bool QHexDocument::canUndo() const { return m_undostack.canUndo(); }
bool QHexDocument::canRedo() const { return m_undostack.canRedo(); }
QHexCursor* QHexDocument::cursor() const { return m_hexcursor; }
const QHexOptions* QHexDocument::options() const { return &m_options; }
QHexMetadata* QHexDocument::metadata() const { return m_hexmetadata; }
qint64 QHexDocument::length() const { return m_buffer->length(); }
quint64 QHexDocument::baseAddress() const { return m_baseaddress; }
uchar QHexDocument::at(int offset) const { return m_buffer->at(offset); }

void QHexDocument::setBaseAddress(quint64 baseaddress)
{
    if(m_baseaddress == baseaddress) return;
    m_baseaddress = baseaddress;
    Q_EMIT changed();
}

void QHexDocument::setOptions(const QHexOptions& options)
{
    auto oldlinelength = m_options.linelength;
    m_options = options;

    if(oldlinelength != m_options.linelength)
        m_hexmetadata->invalidate();

    Q_EMIT changed();
}

void QHexDocument::setByteColor(quint8 b, QHexColor c)
{
    m_options.bytecolors[b] = c;
    Q_EMIT changed();
}

void QHexDocument::setByteForeground(quint8 b, QColor c) { this->setByteColor(b, {c, Qt::transparent}); }
void QHexDocument::setByteBackground(quint8 b, QColor c) { this->setByteColor(b, {Qt::transparent, c}); }
void QHexDocument::undo() { m_undostack.undo(); Q_EMIT changed(); }
void QHexDocument::redo() { m_undostack.redo(); Q_EMIT changed(); }

void QHexDocument::cut(bool hex)
{
    if(!m_hexcursor->hasSelection()) return;
    this->copy(hex);
    m_hexcursor->removeSelection();
}

void QHexDocument::copy(bool hex)
{
    if(!m_hexcursor->hasSelection()) return;

    QClipboard* c = qApp->clipboard();
    QByteArray bytes = m_hexcursor->selectedBytes();
    if(hex) bytes = bytes.toHex(' ').toUpper();
    c->setText(bytes);
}

void QHexDocument::paste(bool hex)
{
    QClipboard* c = qApp->clipboard();
    QByteArray data = c->text().toUtf8();
    if(data.isEmpty()) return;

    m_hexcursor->removeSelection();
    if(hex) data = QByteArray::fromHex(data);

    switch(m_hexcursor->mode())
    {
        case QHexCursor::Mode::Insert: this->insert(m_hexcursor->offset(), data); break;
        default: this->replace(m_hexcursor->offset(), data); break;
    }
}

void QHexDocument::insert(qint64 offset, uchar b) { this->insert(offset, QByteArray(1, b)); }
void QHexDocument::replace(qint64 offset, uchar b) { this->replace(offset, QByteArray(1, b)); }

void QHexDocument::insert(qint64 offset, const QByteArray &data)
{
    m_undostack.push(new InsertCommand(m_buffer, offset, data));
    Q_EMIT changed();
}

void QHexDocument::replace(qint64 offset, const QByteArray &data)
{
    m_undostack.push(new ReplaceCommand(m_buffer, offset, data));
    Q_EMIT changed();
}

void QHexDocument::remove(qint64 offset, int len)
{
    m_undostack.push(new RemoveCommand(m_buffer, offset, len));
    Q_EMIT changed();
}

QByteArray QHexDocument::read(qint64 offset, int len) const { return m_buffer->read(offset, len); }

bool QHexDocument::saveTo(QIODevice *device)
{
    if(!device->isWritable()) return false;
    m_buffer->write(device);
    return true;
}

void QHexDocument::setLineLength(unsigned int l)
{
    if(l == m_options.linelength) return;
    m_options.linelength = l;
    m_hexmetadata->invalidate();
    Q_EMIT changed();
}

void QHexDocument::setGroupLength(unsigned int l)
{
    if(l == m_options.grouplength) return;
    m_options.grouplength = l;
    Q_EMIT changed();
}

qint64 QHexDocument::find(const QByteArray &ba, FindDirection fd) const
{
    qint64 startpos = -1;

    if(m_hexcursor->hasSelection()) startpos = this->cursor()->selectionStartOffset();
    else startpos = this->cursor()->offset();

    if(fd == FindDirection::Backward) startpos--;
    if(startpos < 0) return -1;

    auto offset = fd == FindDirection::Forward ? m_buffer->indexOf(ba, startpos) :
                                                 m_buffer->lastIndexOf(ba, startpos);

    if(offset > -1)
    {
        m_hexcursor->clearSelection();
        m_hexcursor->move(offset);
        m_hexcursor->select(ba.length());
    }

    return offset;
}

QByteArray QHexDocument::getLine(qint64 line) const { return this->read(line * m_options.linelength, m_options.linelength); }

QHexDocument* QHexDocument::fromLargeFile(QString filename, const QHexOptions& options, QObject *parent)
{
    QFile f(filename);
    std::unique_ptr<QHexBuffer> hexbuffer(new QFileBuffer());
    return hexbuffer->read(&f) ? new QHexDocument(hexbuffer.release(), options, parent) : nullptr;
}

QHexDocument* QHexDocument::create(const QHexOptions& options, QObject* parent) { return QHexDocument::fromMemory<QMemoryBuffer>({}, options, parent); }
