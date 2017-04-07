#include "removecommand.h"

RemoveCommand::RemoveCommand(GapBuffer *gapbuffer, integer_t offset, integer_t length, QUndoCommand *parent): HexCommand(gapbuffer, parent)
{
    this->_offset = offset;
    this->_length = length;
}

void RemoveCommand::undo()
{
    this->_gapbuffer->insert(this->_offset, this->_data);
}

void RemoveCommand::redo()
{
    this->_data = this->_gapbuffer->read(this->_offset, this->_length); // Backup data
    this->_gapbuffer->remove(this->_offset, this->_length);
}
