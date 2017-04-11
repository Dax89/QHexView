#ifndef QHEXDOCUMENT_H
#define QHEXDOCUMENT_H

#include <QUndoStack>
#include <QHash>
#include "gapbuffer.h"
#include "metadata/qhexmetadata.h"
#include "qhexcursor.h"

class QHexDocument: public QObject
{
    Q_OBJECT

    private:
        typedef QHash<integer_t, QHexMetadata*> CommentHash;

    private:
        explicit QHexDocument(QIODevice* device, QObject *parent = 0);
        ~QHexDocument();

    public:
        QHexCursor* cursor() const;
        QHexMetadata* metadata() const;
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
        void highlightFore(integer_t startoffset, integer_t endoffset, const QColor &c);
        void highlightBack(integer_t startoffset, integer_t endoffset, const QColor &c);
        void highlightForeRange(integer_t offset, integer_t length, const QColor &c);
        void highlightBackRange(integer_t offset, integer_t length, const QColor &c);
        void comment(integer_t startoffset, integer_t endoffset, const QString& s);
        void commentRange(integer_t offset, integer_t length, const QString& s);
        void clearHighlighting();
        void clearComments();
        void clearMetadata();
        void beginMetadata();
        void endMetadata();
        QByteArray read(integer_t offset, integer_t len) const;
        bool saveTo(QIODevice* device);
        bool isEmpty() const;

    signals:
        void canUndoChanged();
        void canRedoChanged();
        void documentChanged();
        void baseAddressChanged();

    private:
        CommentHash _comments;
        QUndoStack _undostack;
        GapBuffer* _gapbuffer;
        QHexCursor* _cursor;
        QHexMetadata* _metadata;
        integer_t _baseaddress;
};

#endif // QHEXEDITDATA_H
