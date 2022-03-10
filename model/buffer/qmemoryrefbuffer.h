#pragma once

#include "qdevicebuffer.h"

class QMemoryRefBuffer : public QDeviceBuffer
{
    Q_OBJECT

    public:
        explicit QMemoryRefBuffer(QObject *parent = nullptr);
        bool read(QIODevice* device) override;
        void write(QIODevice* device) override;
};
