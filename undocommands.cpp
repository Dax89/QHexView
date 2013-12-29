#include "undocommands.h"

ArrayCommand::ArrayCommand(QHexEditData *hexeditdata, Commands cmd, int pos, QByteArray newBa, int len, QUndoCommand *parent): QUndoCommand(parent)
{
    this->_cmd = cmd;
    this->_hexeditdata = hexeditdata;
    this->_newarray = newBa;
    this->_pos = pos;
    this->_len = len;
}

void ArrayCommand::undo()
{
    switch(this->_cmd)
    {
        case Insert: // Undo Inserted Bytes
            this->_hexeditdata->remove(this->_pos, this->_len);
            break;

        case Replace: // Re-Replace a Replaced Bytes
            this->_hexeditdata->replace(this->_pos, this->_len, this->_bytearray);
            break;

        case Remove: // Insert a Removed Bytes
            this->_hexeditdata->insert(this->_pos, this->_bytearray);
            break;
    }
}

void ArrayCommand::redo()
{
    switch(this->_cmd)
    {
        case Insert:
            this->_hexeditdata->insert(this->_pos, this->_newarray);
            break;

        case Replace:
            this->_bytearray = this->_hexeditdata->read(this->_pos, this->_len);
            this->_hexeditdata->replace(this->_pos, this->_len, this->_newarray);
            break;

        case Remove:
            this->_bytearray = this->_hexeditdata->read(this->_pos, this->_len);
            this->_hexeditdata->remove(this->_pos, this->_len);
            break;
    }
}
