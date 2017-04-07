#ifndef REMOVECOMMAND_H
#define REMOVECOMMAND_H

#include "hexcommand.h"

class RemoveCommand: public HexCommand
{
    public:
        RemoveCommand(GapBuffer* gapbuffer, integer_t offset, integer_t length, QUndoCommand* parent = 0);
        virtual void undo();
        virtual void redo();
};

#endif // REMOVECOMMAND_H
