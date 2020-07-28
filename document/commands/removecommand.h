#ifndef REMOVECOMMAND_H
#define REMOVECOMMAND_H

#include "hexcommand.h"

class RemoveCommand: public HexCommand
{
    public:
        RemoveCommand(QHexBuffer* buffer, quint64 offset, int length, QUndoCommand* parent = nullptr);
        void undo() override;
        void redo() override;
};

#endif // REMOVECOMMAND_H
