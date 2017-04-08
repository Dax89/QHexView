#include "gapbuffer.h"
#include <cstring>

#define BASE_GAP_SIZE 65536 // 64k buffer
//#define BASE_GAP_SIZE 4

GapBuffer::GapBuffer(): _gapstart(0), _gapend(BASE_GAP_SIZE), _gapminlength(BASE_GAP_SIZE)
{
}

GapBuffer::GapBuffer(QIODevice *device): GapBuffer()
{
    this->init(device);
}

GapBuffer::~GapBuffer()
{
}

void GapBuffer::insert(integer_t index, const QByteArray &data)
{
    this->moveGap(index, data.length());

    this->_buffer.replace(this->_gapstart, data.length(), data);
    this->_gapstart += data.length();
}

void GapBuffer::replace(integer_t index, const QByteArray &data)
{
    if(index < this->_gapstart)
    {
        sinteger_t len = this->_gapstart - index;
        this->_buffer.replace(index, len, data.left(len));

        if(len < data.length()) // More data to be copied to the other side of the gap
        {
            index = this->_gapend;
            this->_buffer.replace(index, data.length() - len, data.mid(len));
        }

        return;
    }

    index += this->gapLength();
    this->_buffer.replace(index, data.length(), data);
}

void GapBuffer::remove(integer_t index, integer_t len)
{
    this->moveGap(index, 0);

    #ifdef GAP_BUFFER_TIDY
        this->_buffer.replace(index, len, QByteArray(len, 0x00));
    #endif

    this->_gapend += len;
}

char GapBuffer::at(integer_t index) const
{
    if(index >= this->length())
        throw std::runtime_error("Index out of range");

    if(index >= this->_gapstart)
        index += this->gapLength();

    return this->_buffer.at(index);
}

QByteArray GapBuffer::read(integer_t index, integer_t len) const
{
    if(len == 0)
        len = this->length();

    QByteArray ba;
    ba.reserve(len);

    if(index < this->_gapstart)
    {
        ba.append(this->_buffer.mid(index, std::min(len, this->_gapstart - index)));
        index += ba.length();
        len -= ba.length();
    }

    if(index >= this->_gapstart)
        index += this->_gapend;

    ba.append(this->_buffer.mid(index, len));
    return ba;
}

QByteArray GapBuffer::toByteArray() const
{
    return this->_buffer.mid(0, this->_gapstart) + this->_buffer.mid(this->_gapend);
}

integer_t GapBuffer::length() const
{
    return this->_buffer.length() - this->gapLength();
}

integer_t GapBuffer::gapLength() const
{
    return this->_gapend - this->_gapstart;
}

void GapBuffer::moveGap(integer_t index, integer_t len)
{
    this->expandGap(len);

    if(index == this->_gapstart)
        return;

    sinteger_t delta = index - this->_gapstart;
    char* pdata = this->_buffer.data();

    if(delta < 0) // Move gap left
        std::memcpy(pdata + (this->_gapend + delta), pdata + index, -delta);
    else // Move gap right
        std::memcpy(pdata + this->_gapstart, pdata + this->_gapend, delta);

    this->_gapstart += delta;
    this->_gapend += delta;

    #ifdef GAP_BUFFER_TIDY
        std::memset(pdata + this->_gapstart, 0x00, this->gapLength());
    #endif
}

void GapBuffer::expandGap(integer_t cap)
{
    if(cap <= this->gapLength())
        return;

    if(this->_gapminlength < cap)
        this->_gapminlength = cap * 2;

    sinteger_t delta = this->_gapminlength - this->gapLength();
    this->_buffer.reserve(this->_buffer.length() + delta);

    this->_gapend = this->_gapstart + this->_gapminlength;

    #ifdef GAP_BUFFER_TIDY
        this->_buffer.insert(this->_gapstart, QByteArray(delta, Qt::Uninitialized));
    #else
        this->_buffer.insert(this->_gapstart, QByteArray(delta, char(0)));
    #endif
}

void GapBuffer::init(QIODevice *device)
{
    if(!device->isOpen())
        device->open(QIODevice::ReadWrite);

    #ifdef GAP_BUFFER_TIDY
        this->_buffer.append(QByteArray(device->size() + this->gapLength(), '\0'));
    #else
        this->_buffer.resize(device->size() + this->gapLength());
    #endif

    device->read(this->_buffer.data() + this->gapLength(), device->size());
}
