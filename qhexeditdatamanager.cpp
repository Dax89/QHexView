#include "qhexeditdatamanager.h"

const qint64 QHexEditDataManager::BYTES_PER_LINE = 0x10;

QHexEditDataManager::QHexEditDataManager(QHexEditData *hexeditdata, QObject *parent): QObject(parent)
{
    this->_undostack = new QUndoStack();
    this->_hexeditdata = hexeditdata;

    this->_insMode = Overwrite;
    this->_cursorpos = this->_selectionstart = this->_selectionend = this->_charidx = 0;
}

QHexEditData *QHexEditDataManager::data()
{
    return this->_hexeditdata;
}

qint64 QHexEditDataManager::indexOf(QByteArray &ba, qint64 start)
{
    return this->_hexeditdata->indexOf(ba, start);
}

bool QHexEditDataManager::undo()
{
    if(this->_undostack->canUndo())
    {
        this->_undostack->undo();
        return true;
    }

    return false;
}

bool QHexEditDataManager::redo()
{
    if(this->_undostack->canRedo())
    {
        this->_undostack->redo();
        return true;
    }

    return false;
}

bool QHexEditDataManager::cut()
{
    qint64 start = qMin(this->_selectionstart, this->_selectionend);
    qint64 end = qMax(this->_selectionstart, this->_selectionend);

    if(end - start)
    {
        QClipboard* cpbd = qApp->clipboard();
        QByteArray ba = this->_hexeditdata->read(start, end - start);

        cpbd->setText(QString(ba));
        this->_hexeditdata->remove(start, end - start);
        this->setCursorPos(start, 0);
        return true;
    }

    return false;
}

void QHexEditDataManager::copy()
{
    qint64 start = qMin(this->_selectionstart, this->_selectionend);
    qint64 end = qMax(this->_selectionstart, this->_selectionend);

    if(end - start)
    {
        QClipboard* cpbd = qApp->clipboard();
        QByteArray ba = this->_hexeditdata->read(start, end - start);
        cpbd->setText(QString(ba));
    }
}

bool QHexEditDataManager::paste()
{
    QClipboard* cpbd = qApp->clipboard();
    QString s = cpbd->text();

    if(!s.isNull())
    {
        QByteArray ba = s.toLatin1();
        qint64 start = qMin(this->_selectionstart, this->_selectionend);
        qint64 end = qMax(this->_selectionstart, this->_selectionend);

        if(end - start)
            this->_hexeditdata->replace(start, end - start, ba);
        else
            this->_hexeditdata->insert(start, ba);

        this->setCursorPos((start + s.length()), 0);
        return true;
    }

    return false;
}

void QHexEditDataManager::doAnd(qint64 start, qint64 end, uchar value)
{
    qint64 newstart = qMin(start, end);
    qint64 newend = qMax(start, end);

    if(newend > this->_hexeditdata->length())
        newend = this->_hexeditdata->length();

    QByteArray ba = this->_hexeditdata->read(newstart, newend - newstart);

    for(qint64 i = 0; i < ba.length(); i++)
        ba[(int)i] = ba[(int)i] & value;

    this->replace(newstart, ba);
}

void QHexEditDataManager::doOr(qint64 start, qint64 end, uchar value)
{
    qint64 newstart = qMin(start, end);
    qint64 newend = qMax(start, end);

    if(newend > this->_hexeditdata->length())
        newend = this->_hexeditdata->length();

    QByteArray ba = this->_hexeditdata->read(newstart, newend - newstart);

    for(qint64 i = 0; i < ba.length(); i++)
        ba[(int)i] = ba[(int)i] | value;

    this->replace(newstart, ba);
}

void QHexEditDataManager::doXor(qint64 start, qint64 end, uchar value)
{
    qint64 newstart = qMin(start, end);
    qint64 newend = qMax(start, end);

    if(newend > this->_hexeditdata->length())
        newend = this->_hexeditdata->length();

    QByteArray ba = this->_hexeditdata->read(newstart, newend - newstart);

    for(qint64 i = 0; i < ba.length(); i++)
        ba[(int)i] = ba[(int)i] ^ value;

    this->replace(newstart, ba);
}

void QHexEditDataManager::doMod(qint64 start, qint64 end, uchar value)
{
    qint64 newstart = qMin(start, end);
    qint64 newend = qMax(start, end);

    if(newend > this->_hexeditdata->length())
        newend = this->_hexeditdata->length();

    QByteArray ba = this->_hexeditdata->read(newstart, newend - newstart);

    for(qint64 i = 0; i < ba.length(); i++)
        ba[(int)i] = ba[(int)i] % value;

    this->replace(newstart, ba);
}

void QHexEditDataManager::doNot(qint64 start, qint64 end)
{
    qint64 newstart = qMin(start, end);
    qint64 newend = qMax(start, end);

    if(newend > this->_hexeditdata->length())
        newend = this->_hexeditdata->length();

    QByteArray ba = this->_hexeditdata->read(newstart, newend - newstart);

    for(qint64 i = 0; i < ba.length(); i++)
        ba[(int)i] = !ba[(int)i];

    this->replace(newstart, ba);
}

QHexEditDataManager::InsertMode QHexEditDataManager::mode()
{
    return this->_insMode;
}

void QHexEditDataManager::setMode(QHexEditDataManager::InsertMode im)
{
    this->_insMode = im;
}

QByteArray QHexEditDataManager::read(qint64 pos, qint64 len)
{
    return this->_hexeditdata->read(pos, len);
}

void QHexEditDataManager::setCursorPos(qint64 pos, int charidx)
{
    if(pos > this->_hexeditdata->length()) /* No >= because we can add text at EOF */
    {
        int currLine = this->_selectionstart / QHexEditDataManager::BYTES_PER_LINE;
        int lastLine = this->_hexeditdata->length() / QHexEditDataManager::BYTES_PER_LINE;

        if(currLine == lastLine)
            pos = this->_selectionstart;
        else
            pos = this->_hexeditdata->length();
    }

    this->_cursorpos = this->_selectionend = this->_selectionstart = pos;
    this->_charidx = charidx;
}

qint64 QHexEditDataManager::length()
{
    return this->_hexeditdata->length();
}

qint64 QHexEditDataManager::selectionStart()
{
    return this->_selectionstart;
}

void QHexEditDataManager::setSelectionStart(qint64 pos)
{
    this->_selectionstart = pos;
}

qint64 QHexEditDataManager::selectionEnd()
{
    return this->_selectionend;
}

void QHexEditDataManager::setSelectionEnd(qint64 pos, int charidx)
{
    if(pos >= this->_hexeditdata->length())
        pos = this->_hexeditdata->length();

    this->_selectionend = pos;
    this->_cursorpos = pos;
    this->_charidx = charidx;
}

qint64 QHexEditDataManager::cursorPos()
{
    return this->_cursorpos;
}

int QHexEditDataManager::charIndex()
{
    return this->_charidx;
}

void QHexEditDataManager::insert(qint64 pos, char ch, bool undoable)
{
    this->insert(pos, QByteArray().append(ch), undoable);
}

void QHexEditDataManager::insert(qint64 pos, QByteArray &newba, bool undoable)
{
    if(newba.length())
    {
        if(undoable)
        {
            QUndoCommand* cmd = new ArrayCommand(this->_hexeditdata, ArrayCommand::Insert, pos, newba, newba.length());
            this->_undostack->push(cmd);
        }
        else
            this->_hexeditdata->insert(pos, newba);
    }
}

void QHexEditDataManager::replace(qint64 pos, char ch, bool undoable)
{
    this->replace(pos, QByteArray().append(ch), undoable);
}

void QHexEditDataManager::replace(qint64 pos, QByteArray &newba, bool undoable)
{
    this->replace(pos, newba.length(), newba, undoable);
}

void QHexEditDataManager::replace(qint64 pos, qint64 len, QByteArray &newba, bool undoable)
{
    if(newba.length())
    {
        if(undoable)
        {
            QUndoCommand* cmd = new ArrayCommand(this->_hexeditdata, ArrayCommand::Replace, pos, newba, len);
            this->_undostack->push(cmd);
        }
        else
            this->_hexeditdata->replace(pos, len, newba);
    }
}

void QHexEditDataManager::remove(qint64 pos, qint64 len, bool undoable)
{
    if(len > 0)
    {
        if(undoable)
        {
            QByteArray ba(len, (char)0);
            QUndoCommand* cmd = new ArrayCommand(this->_hexeditdata, ArrayCommand::Remove, pos, ba, len);
            this->_undostack->push(cmd);
        }
        else
            this->_hexeditdata->remove(pos, len);
    }
}
