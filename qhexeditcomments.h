#ifndef QHEXEDITCOMMENTS_H
#define QHEXEDITCOMMENTS_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "sparserangemap.h"

class QHexEditComments : public QObject
{
    Q_OBJECT

    public:
        explicit QHexEditComments(QObject *parent = 0);
        void commentRange(qint64 from, qint64 to, const QString& note);
        void uncommentRange(qint64 from, qint64 to);
        void clearComments();
        bool isCommented(qint64 offset);
        void displayNote(const QPoint &pos, qint64 offset);
        void hideNote();

    private:
        SparseRangeMap<QString> _notes;
};

#endif // QHEXEDITCOMMENTS_H
