#include "insertcommand.h"

InsertCommand::InsertCommand(GapBuffer *gapbuffer, integer_t offset, const QByteArray &data, QUndoCommand *parent): HexCommand(gapbuffer, parent)
{
    this->_offset = offset;
    this->_data = data;
}

void InsertCommand::undo()
{
    this->_gapbuffer->remove(this->_offset, this->_data.length());
}

void InsertCommand::redo()
{
    this->_gapbuffer->insert(this->_offset, this->_data);
}
