#ifndef QHEXMETADATA_H
#define QHEXMETADATA_H

#include <QLinkedList>
#include <QObject>
#include <QColor>
#include <QHash>

struct QHexMetadataAbsoluteItem
{
    qint64 begin;
    qint64 end;
    QColor foreground, background;
    QString comment;
};

struct QHexMetadataItem
{
    int line, start, length;
    QColor foreground, background;
    QString comment;
};

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
typedef QLinkedList<QHexMetadataItem> QHexLineMetadata;
#else
typedef std::list<QHexMetadataItem> QHexLineMetadata;
#endif

class QHexMetadata : public QObject
{
    Q_OBJECT

    public:
        explicit QHexMetadata(QObject *parent = nullptr);
        const QHexLineMetadata& get(int line) const;
        QString comments(int line, int column) const;
        bool hasMetadata(int line) const;

        void clear(int line); // this is transient till next call to setLineWidth()

        void clear();
        void setLineWidth(quint8 width);

    public:
        // new interface with begin, end
        void metadata(qint64 begin, qint64 end, const QColor &fgcolor, const QColor &bgcolor, const QString &comment);

        // old interface with line, start, length
        void metadata(int line, int start, int length, const QColor& fgcolor, const QColor& bgcolor, const QString& comment);
        void color(int line, int start, int length, const QColor& fgcolor, const QColor& bgcolor);
        void foreground(int line, int start, int length, const QColor& fgcolor);
        void background(int line, int start, int length, const QColor& bgcolor);
        void comment(int line, int start, int length, const QString &comment);

    private:
        void setMetadata(const QHexMetadataItem& mi);
        void setAbsoluteMetadata(const QHexMetadataAbsoluteItem& mi);

    signals:
        void metadataChanged(int line);
        void metadataCleared();

    private:
        quint8 m_lineWidth;
        QHash<int, QHexLineMetadata> m_metadata;
        QVector<QHexMetadataAbsoluteItem> m_absoluteMetadata;
};

#endif // QHEXMETADATA_H
