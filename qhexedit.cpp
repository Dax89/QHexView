#include "qhexedit.h"
#include "qhexeditprivate.h"

QHexEdit::QHexEdit(QWidget *parent): QFrame(parent)
{
    this->_vscrollbar = new QScrollBar(Qt::Vertical);
    this->_scrollarea = new QScrollArea();
    this->_hexedit_p = new QHexEditPrivate(this->_scrollarea, this->_vscrollbar);

    connect(this->_hexedit_p, &QHexEditPrivate::verticalScroll, this, &QHexEdit::verticalScroll);
    connect(this->_hexedit_p, &QHexEditPrivate::visibleLinesChanged, this, &QHexEdit::visibleLinesChanged);

    this->_scrollarea->setFocusPolicy(Qt::NoFocus);
    this->_scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Do not show vertical QScrollBar
    this->_scrollarea->setFrameStyle(QFrame::NoFrame);
    this->_scrollarea->setWidgetResizable(true);
    this->_scrollarea->setWidget(this->_hexedit_p);

    this->setFocusPolicy(Qt::NoFocus);
    this->setFocusProxy(this->_hexedit_p);

    this->_hlayout = new QHBoxLayout();
    this->_hlayout->setSpacing(0);
    this->_hlayout->setMargin(0);
    this->_hlayout->addWidget(this->_scrollarea);
    this->_hlayout->addWidget(this->_vscrollbar);

    this->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    this->setLayout(this->_hlayout);
}

QHexDocument *QHexEdit::document() const
{
    return this->_hexedit_p->document();
}

QHexMetrics *QHexEdit::metrics() const
{
    return this->_hexedit_p->metrics();
}

bool QHexEdit::readOnly() const
{
    return this->_hexedit_p->readOnly();
}

void QHexEdit::setReadOnly(bool b)
{
    this->_hexedit_p->setReadOnly(b);
}

void QHexEdit::setDocument(QHexDocument *document)
{
    this->_hexedit_p->setDocument(document);
}

void QHexEdit::scroll(QWheelEvent *event)
{
    this->_hexedit_p->scroll(event);
}
