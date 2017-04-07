#ifndef HEXCOMMAND_H
#define HEXCOMMAND_H

#include <QUndoCommand>
#include "../gapbuffer.h"

class HexCommand: public QUndoCommand
{
    public:
        HexCommand(GapBuffer* gapbuffer, QUndoCommand* parent = 0);

    protected:
        GapBuffer* _gapbuffer;
        integer_t _offset, _length;
        QByteArray _data;
};

#endif // HEXCOMMAND_H
