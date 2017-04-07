#include "replacecommand.h"

ReplaceCommand::ReplaceCommand(GapBuffer *gapbuffer, integer_t offset, const QByteArray &data, QUndoCommand *parent): HexCommand(gapbuffer, parent)
{
    this->_offset = offset;
    this->_data = data;
}

void ReplaceCommand::undo()
{
    this->_gapbuffer->replace(this->_offset, this->_olddata);
}

void ReplaceCommand::redo()
{
    this->_olddata = this->_gapbuffer->read(this->_offset, this->_data.length());
    this->_gapbuffer->replace(this->_offset, this->_data);
}
