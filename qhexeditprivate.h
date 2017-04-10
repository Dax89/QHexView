#ifndef QHEXEDITPRIVATE_H
#define QHEXEDITPRIVATE_H

#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>
#include "qhexedit.h"
#include "document/qhexdocument.h"
#include "document/qhextheme.h"
#include "paint/qhexmetrics.h"

class QHexEditPrivate : public QWidget
{
    Q_OBJECT

    public:
        explicit QHexEditPrivate(QScrollArea* scrollarea, QScrollBar* vscrollbar, QWidget *parent = 0);
        QHexDocument* document() const;
        QHexMetrics* metrics() const;
        bool readOnly() const;
        void setDocument(QHexDocument* document);
        void setReadOnly(bool b);
        void scroll(QWheelEvent *event);

    private:
        integer_t offsetFromPoint(const QPoint& pt, integer_t* bitindex = NULL) const;
        void toggleComment(const QPoint &pos);
        void updateCaret(integer_t offset, integer_t nibbleindex);
        void processDeleteEvents();
        void processBackspaceEvents();
        void processHexPart(int key);
        void processAsciiPart(int key);
        bool processMoveEvents(QKeyEvent* event);
        bool processSelectEvents(QKeyEvent* event);
        bool processTextInputEvents(QKeyEvent* event);
        bool processInsOvrEvents(QKeyEvent* event);
        bool processUndoRedo(QKeyEvent* event);
        bool processClipboardKeys(QKeyEvent* event);

    protected:
        void paintEvent(QPaintEvent* pe);
        void mousePressEvent(QMouseEvent* event);
        void mouseMoveEvent(QMouseEvent* event);
        void wheelEvent(QWheelEvent* event);
        void keyPressEvent(QKeyEvent* event);
        void resizeEvent(QResizeEvent*e);

    signals:
        void visibleLinesChanged();
        void verticalScroll(int value);

    private:
        static const integer_t WHELL_SCROLL_LINES;
        QScrollArea* _scrollarea;
        QScrollBar* _vscrollbar;
        QHexDocument* _document;
        QHexMetrics* _metrics;
        QHexTheme* _theme;
        bool _readonly;
};

#endif // QHEXEDITPRIVATE_H
