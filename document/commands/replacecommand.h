#ifndef REPLACECOMMAND_H
#define REPLACECOMMAND_H

#include "hexcommand.h"

class ReplaceCommand: public HexCommand
{
    public:
        ReplaceCommand(GapBuffer* gapbuffer, integer_t offset, const QByteArray& data, QUndoCommand* parent = 0);
        virtual void undo();
        virtual void redo();

    private:
        QByteArray _olddata;
};

#endif // REPLACECOMMAND_H
