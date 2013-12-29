#ifndef QHEXEDITDATAMAGER_H
#define QHEXEDITDATAMAGER_H

#include <QtCore>
#include <QtWidgets>
#include <QUndoStack>
#include "qhexeditdata.h"
#include "undocommands.h"
#include "piece.h"

/*
 * TODO: QHexEditBuffer: Large File Support (writing)
 */
class QHexEditDataManager : public QObject
{
    Q_OBJECT

    public:
        enum InsertMode {Overwrite = 0, Insert = 1};
        explicit QHexEditDataManager(QHexEditData* hexeditdata, QObject *parent = 0);
        QHexEditData* data();
        qint64 indexOf(QByteArray& ba, qint64 start);
        bool undo();
        bool redo();
        bool cut();
        void copy();
        bool paste();
        void doAnd(qint64 start, qint64 end, uchar value);
        void doOr(qint64 start, qint64 end, uchar value);
        void doXor(qint64 start, qint64 end, uchar value);
        void doMod(qint64 start, qint64 end, uchar value);
        void doNot(qint64 start, qint64 end);
        QHexEditDataManager::InsertMode mode();
        void setMode(QHexEditDataManager::InsertMode im);
        QByteArray read(qint64 pos, qint64 len);
        void setCursorPos(qint64 pos, int charidx);
        qint64 length();
        qint64 selectionStart();
        void setSelectionStart(qint64 pos);
        qint64 selectionEnd();
        void setSelectionEnd(qint64 pos, int charidx);
        qint64 cursorPos();
        int charIndex();

    public: /* Editing Interface */
        void insert(qint64 pos, char ch, bool undoable = true);
        void insert(qint64 pos, QByteArray& newba, bool undoable = true);
        void replace(qint64 pos, char ch, bool undoable = true);
        void replace(qint64 pos, QByteArray& newba, bool undoable = true);
        void replace(qint64 pos, qint64 len, QByteArray& newba, bool undoable = true);
        void remove(qint64 pos, qint64 len, bool undoable = true);

    public: /* Constants */
        static const qint64 BYTES_PER_LINE;

    private:
        QHexEditData* _hexeditdata;
        QUndoStack* _undostack;
        InsertMode _insMode;
        qint64 _selectionstart;
        qint64 _selectionend;
        qint64 _cursorpos;
        int _charidx;
};

#endif // QHEXEDITDATAMAGER_H

