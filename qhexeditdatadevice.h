#ifndef QHEXEDITDATADEVICE_H
#define QHEXEDITDATADEVICE_H

#include <QtCore>
#include "qhexeditdata.h"
#include "qhexeditdatareader.h"
#include "qhexeditdatawriter.h"

class QHexEditDataDevice : public QIODevice
{
    public:
        QHexEditDataDevice(QHexEditData* hexeditdata);
        virtual qint64 size();

    protected:
        qint64 readData(char *data, qint64 maxlen);
        qint64 writeData(const char *data, qint64 len);

    private:
        QHexEditData* _hexeditdata;
        QHexEditDataReader* _reader;
        QHexEditDataWriter* _writer;
};

#endif // QHEXEDITDATADEVICE_H
