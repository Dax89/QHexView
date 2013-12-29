#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QByteArray>
#include "qhexeditdata.h"

class ArrayCommand: public QUndoCommand
{
    public:
        enum Commands {Insert, Replace, Remove};
        explicit ArrayCommand(QHexEditData* hexeditdata, Commands cmd, int pos, QByteArray newBa = QByteArray(), int len = 0, QUndoCommand* parent = NULL);
        void undo();
        void redo();

    private:
        Commands _cmd;
        QHexEditData* _hexeditdata;
        QByteArray _newarray;
        QByteArray _bytearray;
        int _pos;
        int _len;
};

#endif // UNDOCOMMANDS_H
