#ifndef GAPBUFFER_H
#define GAPBUFFER_H

#include <QByteArray>
#include <QIODevice>

//#define GAP_BUFFER_TIDY

typedef int64_t  sinteger_t;
typedef uint64_t integer_t;

class GapBuffer
{
    public:
        GapBuffer();
        GapBuffer(QIODevice* device);
        ~GapBuffer();

    public:
        void insert(integer_t index, const QByteArray& data);
        void replace(integer_t index, const QByteArray& data);
        void remove(integer_t index, integer_t len);
        char at(integer_t index) const;
        QByteArray read(integer_t index, integer_t len = 0) const;
        QByteArray toByteArray() const;
        integer_t length() const;

    private:
        integer_t gapLength() const;
        void moveGap(integer_t index, integer_t len);
        void expandGap(integer_t cap);
        void init(QIODevice* device);

    private:
        integer_t _gapstart, _gapend, _gapminlength;
        QByteArray _buffer;
};

#endif // GAPBUFFER_H
