#ifndef QHEXDOCUMENT_H
#define QHEXDOCUMENT_H

#include <QUndoStack>
#include "gapbuffer.h"
#include "qhexcursor.h"

class QHexDocument: public QObject
{
    Q_OBJECT

    private:
        explicit QHexDocument(QIODevice* device, QObject *parent = 0);
        ~QHexDocument();

    public:
        QHexCursor* cursor() const;
        integer_t length() const;
        integer_t baseAddress() const;
        bool canUndo() const;
        bool canRedo() const;
        QByteArray read(integer_t offset, integer_t len = 0);
        QByteArray selectedBytes() const;
        char at(integer_t offset) const;
        void setBaseAddress(integer_t baseaddress);

    public:
        static QHexDocument* fromDevice(QIODevice* iodevice);
        static QHexDocument* fromFile(QString filename);
        static QHexDocument* fromMemory(const QByteArray& ba);

    public slots:
        void undo();
        void redo();
        void cut();
        void copy();
        void paste();
        void insert(integer_t offset, uchar b);
        void replace(integer_t offset, uchar b);
        void insert(integer_t offset, const QByteArray& data);
        void replace(integer_t offset, const QByteArray& data);
        void remove(integer_t offset, integer_t len);
        QByteArray read(integer_t offset, integer_t len) const;
        bool saveTo(QIODevice* device);
        bool isEmpty() const;

    signals:
        void canUndoChanged();
        void canRedoChanged();
        void documentChanged();
        void baseAddressChanged();

    private:
        QUndoStack _undostack;
        GapBuffer* _gapbuffer;
        QHexCursor* _cursor;
        integer_t _baseaddress;
};

#endif // QHEXEDITDATA_H
