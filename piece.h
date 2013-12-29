#ifndef PIECE_H
#define PIECE_H

#include <QtCore>

struct Piece
{
    quint64 Offset;
    quint64 Length;
    qint64 BufferId;
};

#endif // PIECE_H
