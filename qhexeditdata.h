#ifndef QHEXEDITDATA_H
#define QHEXEDITDATA_H

#include <QtCore>

class QHexEditData : public QObject
{
    Q_OBJECT

    public:
        explicit QHexEditData(QIODevice* iodevice, QObject *parent = 0);
        ~QHexEditData();
        const char *constDataPtr();
        char* dataPtr();
        uchar at(qint64 i);
        qint64 indexOf(const QByteArray& ba, qint64 start);
        QByteArray read(qint64 pos, qint64 len);
        qint64 length();
        void save(QString filename);

    public: /* Editing Interface */
        void write(const QByteArray& ba);
        void insert(qint64 pos, const QByteArray& newba);
        void replace(qint64 pos, qint64 len, const QByteArray& newba);
        void remove(qint64 pos, qint64 len);

    signals:
        void dataChanged(qint64 offset, qint64 length);

    private:
        QIODevice* _iodevice;
        QByteArray _bytes;
};

#endif // QHEXEDITDATA_H
