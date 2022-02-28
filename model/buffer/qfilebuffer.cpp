#include "qfilebuffer.h"
#include <QFile>
#include <limits>

QFileBuffer::QFileBuffer(QObject *parent) : QHexBuffer(parent) { }

QFileBuffer::~QFileBuffer()
{
    if(!m_file) return;
    if(m_file->isOpen()) m_file->close();
    if(m_file->parent() != this) m_file->deleteLater();
    m_file = nullptr;
}

uchar QFileBuffer::at(qint64 idx)
{
    m_file->seek(idx);

    char c = '\0';
    m_file->getChar(&c);

    return static_cast<uchar>(c);
}

qint64 QFileBuffer::length() const { return m_file->size(); }

void QFileBuffer::insert(qint64 offset, const QByteArray &data) {
    Q_UNUSED(offset)
    Q_UNUSED(data)
    // Not implemented
}

void QFileBuffer::remove(qint64 offset, int length) {
    Q_UNUSED(offset)
    Q_UNUSED(length)
    // Not implemented
}

QByteArray QFileBuffer::read(qint64 offset, int length) {
    m_file->seek(offset);
    return m_file->read(length);
}

bool QFileBuffer::read(QIODevice *device) {
    m_file = qobject_cast<QFile*>(device);
    if(!m_file) return false;

    m_file->setParent(this);
    if(!m_file->isOpen()) m_file->open(QIODevice::ReadWrite);
    return m_file->isOpen();
}

void QFileBuffer::write(QIODevice *device) {
    Q_UNUSED(device)
    // Not implemented
}

qint64 QFileBuffer::indexOf(const QByteArray& ba, qint64 from)
{
    const auto MAX = std::numeric_limits<int>::max();
    qint64 idx = -1;

    if(from < m_file->size())
    {
        idx = from;
        m_file->seek(from);

        while(idx < m_file->size())
        {
            QByteArray data = m_file->read(MAX);
            int sidx = data.indexOf(ba);

            if(sidx >= 0)
            {
                idx += sidx;
                break;
            }

            if(idx + data.size() >= m_file->size()) return -1;
            m_file->seek(m_file->pos() + data.size() - ba.size());
        }
    }

    return idx;
}

qint64 QFileBuffer::lastIndexOf(const QByteArray& ba, qint64 from)
{
    const auto MAX = std::numeric_limits<int>::max();
    qint64 idx = -1;

    if(from >= 0 && ba.size() < MAX) {
        qint64 currpos = from;

        while(currpos >= 0)
        {
            qint64 readpos = (currpos < MAX) ? 0 : currpos - MAX;
            m_file->seek(readpos);

            QByteArray data = m_file->read(currpos - readpos);
            int lidx = data.lastIndexOf(ba, from);

            if(lidx >= 0)
            {
                idx = readpos + lidx;
                break;
            }

            if(readpos <= 0) break;
            currpos = readpos + ba.size();
        }

    }

    return idx;
}
