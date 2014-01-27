#ifndef QHEXEDITHIGHLIGHTER_H
#define QHEXEDITHIGHLIGHTER_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexeditdata.h"

class QHexEditHighlighter : public QObject
{
    Q_OBJECT

    private:
        class ColorRange
        {
            public:
                explicit ColorRange(qint64 p, qint64 l, QColor c): _pos(p), _len(l), _color(c) { }
                ColorRange(const ColorRange& cr): _pos(cr._pos), _len(cr._len), _color(cr._color) { }
                static ColorRange invalid() { return ColorRange(-1, -1, QColor(QColor::Invalid)); }

            public:
                bool contains(qint64 pos) const { return (pos >= this->_pos) && (pos <= this->endPos()); }
                void setColor(const QColor& c) { this->_color = c; }
                const QColor& color() const { return this->_color; }
                qint64 pos() const { return this->_pos; }
                qint64 endPos() const { return this->_pos + this->_len; }
                qint64 length() const { return this->_len; }
                bool isValid() const { return (this->_pos != -1) && (this->_len != -1) && this->_color.isValid(); }

            private:
                qint64 _pos;
                qint64 _len;
                QColor _color;
        };

    private:
        typedef QList<ColorRange> ColorList;

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
        bool canOptimize(const QHexEditHighlighter::ColorRange cr, qint64 start, qint64 end, const QColor& color) const;
        ColorRange rangeAt(const QHexEditHighlighter::ColorList &rangelist, qint64 pos, qint64* index = nullptr) const;
        ColorRange insertionPoint(const QHexEditHighlighter::ColorList &rangelist, qint64 pos, qint64 *index = nullptr) const;
        void internalClear(QHexEditHighlighter::ColorList& rangelist, qint64 start, qint64 end);
        void internalHighlight(QHexEditHighlighter::ColorList& rangelist, qint64 start, qint64 end, const QColor& color);

    private:
        QHexEditHighlighter::ColorList _backgroundranges;
        QHexEditHighlighter::ColorList _foregroundranges;
        QHexEditData* _hexeditdata;
        QColor _backdefault;
        QColor _foredefault;
};

#endif // QHEXEDITHIGHLIGHTER_H
