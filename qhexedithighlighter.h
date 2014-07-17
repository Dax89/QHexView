#ifndef QHEXEDITHIGHLIGHTER_H
#define QHEXEDITHIGHLIGHTER_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexeditdata.h"
#include "sparserangemap.h"

class QHexEditHighlighter : public QObject
{
    Q_OBJECT

    private:
        typedef SparseRangeMap<QColor> ColorMap;

    public:
        explicit QHexEditHighlighter(QHexEditData* hexeditdata, QColor backdefault, QColor foredefault, QObject *parent = 0);
        void colors(qint64 pos, QColor& bc, QColor& fc) const;
        QColor defaultBackColor() const;
        QColor defaultForeColor() const;
        QColor backColor(qint64 pos) const;
        QColor foreColor(qint64 pos) const;
        void highlightForeground(qint64 start, qint64 end, const QColor& color);
        void highlightBackground(qint64 start, qint64 end, const QColor& color);
        void clearHighlight(qint64 start, qint64 end);

    private:
        ColorMap _backgroundmap;
        ColorMap _foregroundmap;
        QHexEditData* _hexeditdata;
        QColor _backdefault;
        QColor _foredefault;
};

#endif // QHEXEDITHIGHLIGHTER_H
