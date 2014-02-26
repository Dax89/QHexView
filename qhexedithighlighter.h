#ifndef QHEXEDITHIGHLIGHTER_H
#define QHEXEDITHIGHLIGHTER_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexeditdata.h"

class QHexEditHighlighter : public QObject
{
    Q_OBJECT

    /*
    private:
        class ColorRange
        {
            public:
                explicit ColorRange(qint64 p, qint64 e, QColor c): _start(p), _end(e), _color(c) { }
                ColorRange(const ColorRange& cr): _start(cr._start), _end(cr._end), _color(cr._color) { }
                static ColorRange invalid() { return ColorRange(-1, -1, QColor(QColor::Invalid)); }

            public:
                bool contains(qint64 pos) const { return (pos >= this->_start) && (pos <= this->end()); }
                void setColor(const QColor& c) { this->_color = c; }
                const QColor& color() const { return this->_color; }
                qint64 start() const { return this->_start; }
                qint64 end() const { return this->_end; }
                qint64 length() const { return this->_end - this->_start; }
                bool isValid() const { return (this->_start != -1) && (this->_end != -1) && this->_color.isValid(); }

            private:
                qint64 _start;
                qint64 _end;
                QColor _color;
        };
     */

    private:
        typedef QHash<qint64, QColor> ColorMap;

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
        void internalClear(ColorMap &rangelist, qint64 start, qint64 end);
        void internalHighlight(QHexEditHighlighter::ColorMap& rangelist,qint64 start, qint64 end, const QColor& color);

    private:
        QHexEditHighlighter::ColorMap _backgroundmap;
        QHexEditHighlighter::ColorMap _foregroundmap;
        QHexEditData* _hexeditdata;
        QColor _backdefault;
        QColor _foredefault;
};

#endif // QHEXEDITHIGHLIGHTER_H
