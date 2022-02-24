#include "qhexview.h"
#include "document/qhexcursor.h"
#include <QMouseEvent>
#include <QFontDatabase>
#include <QApplication>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollBar>
#include <QPalette>
#include <QPainter>
#include <climits>
#include <cctype>
#include <cmath>

#include <QDebug>

QHexView::QHexView(QWidget *parent) : QAbstractScrollArea(parent), m_fontmetrics(this->font())
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    if(f.styleHint() != QFont::TypeWriter)
    {
        f.setFamily("Monospace"); // Force Monospaced font
        f.setStyleHint(QFont::TypeWriter);
    }

    this->setFont(f);
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);
    this->viewport()->setCursor(Qt::IBeamCursor);

    QPalette p = this->palette();
    p.setBrush(QPalette::Window, p.base());

    this->checkState();
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int) { this->viewport()->update(); });
}

QHexDocument* QHexView::hexDocument() const { return m_hexdocument; }
QHexCursor* QHexView::hexCursor() const { return m_hexdocument->cursor(); }
const QHexOptions& QHexView::options() const { return m_hexdocument->options(); }
void QHexView::setOptions(const QHexOptions& options) { if(m_hexdocument) m_hexdocument->setOptions(options); }

void QHexView::setDocument(QHexDocument* doc)
{
    m_writing = false;

    if(m_hexdocument)
    {
        disconnect(m_hexdocument->cursor(), &QHexCursor::positionChanged, this, nullptr);
        disconnect(m_hexdocument->cursor(), &QHexCursor::modeChanged, this, nullptr);
        disconnect(m_hexdocument, &QHexDocument::changed, this, nullptr);
    }

    m_hexdocument = doc;
    connect(m_hexdocument, &QHexDocument::changed, this, [=]() { this->checkAndUpdate(true); });
    connect(m_hexdocument->cursor(), &QHexCursor::positionChanged, this, [=]() { m_writing = false; this->viewport()->update(); });
    connect(m_hexdocument->cursor(), &QHexCursor::modeChanged, this, [=]() { m_writing = false; this->viewport()->update(); });
    this->checkAndUpdate(true);
}

void QHexView::setReadOnly(bool r) { m_readonly = r; }
void QHexView::setLineLength(int l) { m_hexdocument->setLineLength(l); this->checkAndUpdate(true); }
void QHexView::setGroupLength(int l) { m_hexdocument->setGroupLength(l); }

void QHexView::checkState()
{
    if(!m_hexdocument) return;
    m_hexdocument->checkOptions(this->palette());

    this->verticalScrollBar()->setSingleStep(this->options().scrollsteps);
    this->verticalScrollBar()->setPageStep(this->options().scrollsteps);

    quint64 doclines = m_hexdocument->lines();
    int vislines = this->visibleLines();

    if(doclines > static_cast<quint64>(vislines))
    {
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        this->verticalScrollBar()->setMaximum(static_cast<int>((doclines - vislines) / this->documentSizeFactor() + 1));
    }
    else
    {
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->verticalScrollBar()->setMaximum(doclines);
    }
}

void QHexView::checkAndUpdate(bool calccolumns)
{
    this->checkState();
    if(calccolumns) this->calcColumns();
    this->viewport()->update();
}

void QHexView::calcColumns()
{
    m_hexcolumns.clear();
    m_hexcolumns.reserve(this->options().linelength);

    auto x = this->hexColumnX(), cw = this->cellWidth() * 2;

    for(auto i = 0u; i < this->options().linelength; i++)
    {
        for(auto j = 0u; j < this->options().grouplength; j++, x += cw)
            m_hexcolumns.push_back(QRect(x, 0, cw, 0));

        x += this->cellWidth();
    }
}

void QHexView::drawHeader(QTextCursor& c) const
{
    QString addressheader = this->options().addresslabel.rightJustified(this->addressWidth()), hexheader;

    for(auto i = 0u; i < this->options().linelength; i += this->options().grouplength)
        hexheader.append(QString("%1 ").arg(QString::number(i, 16).rightJustified(this->options().grouplength * 2, '0')).toUpper());

    QTextCharFormat cf;
    cf.setForeground(this->options().headercolor);

    c.insertText(m_fontmetrics.elidedText(addressheader, Qt::ElideRight, this->hexColumnX()) + " ", cf);
    c.insertText(hexheader, cf);
    c.insertText(this->options().asciilabel, cf);

    c.insertBlock();
}

void QHexView::drawDocument(QTextCursor& c) const
{
    if(!m_hexdocument) return;

    qreal y = this->options().header ? this->lineHeight() : 0;
    quint64 line = static_cast<quint64>(this->verticalScrollBar()->value());

    QTextCharFormat addrformat;
    addrformat.setForeground(this->palette().color(QPalette::Normal, QPalette::Highlight));

    for(qint64 l = 0; (line < m_hexdocument->lines()) && (l < this->visibleLines()); l++, line++, y += this->lineHeight())
    {
        quint64 address = line * this->options().linelength + m_hexdocument->baseAddress();
        QString addrstr = QString::number(address, 16).rightJustified(this->addressWidth(), '0').toUpper();

        // Address Part
        QTextCharFormat acf;
        acf.setForeground(this->options().headercolor);
        c.insertText(addrstr + " ", acf);

        auto linebytes = m_hexdocument->getLine(line);

        // Hex Part
        for(auto column = 0u; column < this->options().linelength; )
        {
            QTextCharFormat cf;

            for(auto byteidx = 0u; byteidx < this->options().grouplength; byteidx++, column++)
                cf = this->drawFormat(c, linebytes.mid(column, 1).toHex().toUpper(), Area::Hex, line, column);

            c.insertText(" ", cf);
        }

        // Ascii Part
        for(auto column = 0u; column < this->options().linelength; column++)
        {
            this->drawFormat(c, std::isprint(linebytes[column]) ? QChar(linebytes[column]) : this->options().unprintablechar,
                             Area::Ascii, line, column);
        }

        c.insertBlock();
    }
}

int QHexView::documentSizeFactor() const
{
    const auto M = std::numeric_limits<qint64>::max();
    int factor = 1;

    if(m_hexdocument)
    {
        quint64 doclines = m_hexdocument->lines();
        if(doclines >= M) factor = static_cast<int>(doclines / M) + 1;
    }

    return factor;
}

int QHexView::visibleLines() const
{
    quint64 vl = std::ceil(this->height() / this->lineHeight());
    if(this->options().header) vl--;
    return static_cast<int>(std::min(m_hexdocument->lines(), vl));
}

qreal QHexView::hexColumnWidth() const
{
    int l = 0;

    for(auto i = 0u; i < this->options().linelength; i += this->options().grouplength)
        l += (1 << this->options().grouplength) + 1;

    return this->getNCellsWidth(l);
}

qreal QHexView::addressWidth() const { return 8; }
qreal QHexView::hexColumnX() const { return this->getNCellsWidth(this->addressWidth()) + this->cellWidth(); }
qreal QHexView::asciiColumnX() const { return this->hexColumnX() + this->hexColumnWidth(); }
qreal QHexView::endColumnX() const { return this->asciiColumnX() + this->cellWidth() + this->getNCellsWidth(this->options().linelength); }
qreal QHexView::getNCellsWidth(int n) const { return n * this->cellWidth(); }

qreal QHexView::cellWidth() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return m_fontmetrics.horizontalAdvance(" ");
#else
    return m_fontmetrics.width(" ");
#endif
}

qreal QHexView::lineHeight() const { return m_fontmetrics.height(); }

QHexCursor::Position QHexView::positionFromPoint(QPoint pt) const
{
    QHexCursor::Position pos = QHexCursor::Position::invalid();
    pt = this->absolutePoint(pt);

    switch(this->areaFromPoint(pt))
    {
        case Area::Hex: {
            pos.column = -1;

            for(qint64 i = 0; i < m_hexcolumns.size(); i++) {
                if((pt.x() >= m_hexcolumns.at(i).left()) && (pt.x() <= m_hexcolumns.at(i).right())) {
                    pos.column = i;
                    break;
                }
            }

            break;
        }

        case Area::Ascii: pos.column = std::floor((pt.x() - this->asciiColumnX()) / this->cellWidth()); break;
        case Area::Address: pos.column = 0; break;
        case Area::Header: return QHexCursor::Position::invalid();
        default: break;
    }

    pos.line = std::min<qint64>(this->verticalScrollBar()->value() + (pt.y() / this->lineHeight()), m_hexdocument->lines());
    if(this->options().header) pos.line = std::max<qint64>(0, pos.line - 1);
    return pos;
}

QPoint QHexView::absolutePoint(QPoint pt) const
{
    QPoint shift(this->horizontalScrollBar()->value(), 0);
    return pt + shift;
}

QHexView::Area QHexView::areaFromPoint(QPoint pt) const
{
    qreal line = this->verticalScrollBar()->value() + pt.y() / this->lineHeight();
    qreal x = std::ceil(this->horizontalScrollBar()->value() + pt.x());

    if(this->options().header && !std::floor(line)) return Area::Header;
    if(x < this->hexColumnX()) return Area::Address;
    if(x < this->asciiColumnX()) return Area::Hex;
    if(x < this->endColumnX()) return Area::Ascii;
    return Area::Extra;
}

QTextCharFormat QHexView::drawFormat(QTextCursor& c, const QString& s, Area area, qint64 line, qint64 column) const
{
    auto cursorbg = this->palette().color(QPalette::Normal, QPalette::WindowText);
    auto cursorfg = this->palette().color(QPalette::Normal, QPalette::Window);
    auto discursorbg = this->palette().color(QPalette::Disabled, QPalette::WindowText);
    auto discursorfg = this->palette().color(QPalette::Disabled, QPalette::Window);

    QTextCharFormat cf, selcf;

    if(this->hexCursor()->isSelected(line, column))
    {
        cf.setBackground(this->palette().color(QPalette::Normal, QPalette::Highlight));
        cf.setForeground(this->palette().color(QPalette::Normal, QPalette::HighlightedText));
        if(column < m_hexdocument->lastColumn() - 1) selcf = cf;
    }

    if(this->hexCursor()->line() == line && this->hexCursor()->column() == column)
    {
        switch(m_hexdocument->cursor()->mode())
        {
            case QHexCursor::Mode::Insert:
                cf.setUnderlineColor(m_currentarea == area ? cursorbg : discursorbg);
                cf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::SingleUnderline);
                break;

            case QHexCursor::Mode::Overwrite:
                cf.setBackground(m_currentarea == area ? cursorbg : discursorbg);
                cf.setForeground(m_currentarea == area ? cursorfg : discursorfg);
                break;
        }
    }

    c.insertText(s, cf);
    return selcf;
}

void QHexView::moveNext(bool select)
{
    auto line = this->hexCursor()->line(), column = this->hexCursor()->column();

    if(column >= this->options().linelength - 1)
    {
        line++;
        column = 0;
    }
    else
        column++;

    if(select) this->hexCursor()->select(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->lastColumn()));
    else this->hexCursor()->move(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->lastColumn()));
}

void QHexView::movePrevious(bool select)
{
    auto line = this->hexCursor()->line(), column = this->hexCursor()->column();

    if(column <= 0)
    {
        if(!line) return;
        column = m_hexdocument->getLine(--line).size() - 1;
    }
    else
        column--;

    if(select) this->hexCursor()->select(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->lastColumn()));
    else this->hexCursor()->move(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->lastColumn()));
}

bool QHexView::keyPressMove(QKeyEvent* e)
{
    if(e->matches(QKeySequence::MoveToNextChar) || e->matches(QKeySequence::SelectNextChar))
        this->moveNext(e->matches(QKeySequence::SelectNextChar));
    else if(e->matches(QKeySequence::MoveToPreviousChar) || e->matches(QKeySequence::SelectPreviousChar))
        this->movePrevious(e->matches(QKeySequence::SelectPreviousChar));
    else if(e->matches(QKeySequence::MoveToNextLine) || e->matches(QKeySequence::SelectNextLine))
    {
        if(m_hexdocument->lastLine() == this->hexCursor()->line()) return true;
        auto nextline = this->hexCursor()->line() + 1;
        if(e->matches(QKeySequence::MoveToNextLine)) this->hexCursor()->move(nextline, this->hexCursor()->column());
        else this->hexCursor()->select(nextline, this->hexCursor()->column());
    }
    else if(e->matches(QKeySequence::MoveToPreviousLine) || e->matches(QKeySequence::SelectPreviousLine))
    {
        if(!this->hexCursor()->line()) return true;
        auto prevline = this->hexCursor()->line() - 1;
        if(e->matches(QKeySequence::MoveToPreviousLine)) this->hexCursor()->move(prevline, this->hexCursor()->column());
        else this->hexCursor()->select(prevline, this->hexCursor()->column());
    }
    else if(e->matches(QKeySequence::MoveToNextPage) || e->matches(QKeySequence::SelectNextPage))
    {
        if(m_hexdocument->lastLine() == this->hexCursor()->line()) return true;
        auto pageline = std::min(m_hexdocument->lastLine(), this->hexCursor()->line() + this->visibleLines());
        if(e->matches(QKeySequence::MoveToNextPage)) this->hexCursor()->move(pageline, this->hexCursor()->column());
        else this->hexCursor()->select(pageline, this->hexCursor()->column());
    }
    else if(e->matches(QKeySequence::MoveToPreviousPage) || e->matches(QKeySequence::SelectPreviousPage))
    {
        if(!this->hexCursor()->line()) return true;
        auto pageline = std::max<qint64>(0, this->hexCursor()->line() - this->visibleLines());
        if(e->matches(QKeySequence::MoveToPreviousPage)) this->hexCursor()->move(pageline, this->hexCursor()->column());
        else this->hexCursor()->select(pageline, this->hexCursor()->column());
    }
    else if(e->matches(QKeySequence::MoveToStartOfDocument) || e->matches(QKeySequence::SelectStartOfDocument))
    {
        if(!this->hexCursor()->line()) return true;
        if(e->matches(QKeySequence::MoveToStartOfDocument)) this->hexCursor()->move(0, 0);
        else this->hexCursor()->select(0, 0);
    }
    else if(e->matches(QKeySequence::MoveToEndOfDocument) || e->matches(QKeySequence::SelectEndOfDocument))
    {
        if(m_hexdocument->lastLine() == this->hexCursor()->line()) return true;
        if(e->matches(QKeySequence::MoveToEndOfDocument)) this->hexCursor()->move(m_hexdocument->lastLine(), m_hexdocument->lastColumn());
        else this->hexCursor()->select(m_hexdocument->lastLine(), m_hexdocument->lastColumn());
    }
    else if(e->matches(QKeySequence::MoveToStartOfLine) || e->matches(QKeySequence::SelectStartOfLine))
    {
        if(e->matches(QKeySequence::MoveToStartOfLine)) this->hexCursor()->move(this->hexCursor()->line(), 0);
        else this->hexCursor()->select(this->hexCursor()->line(), 0);
    }
    else if(e->matches(QKeySequence::MoveToEndOfLine) || e->matches(QKeySequence::SelectEndOfLine))
    {
        if(e->matches(QKeySequence::MoveToEndOfLine))
        {
            if(this->hexCursor()->line() == m_hexdocument->lastLine()) this->hexCursor()->move(this->hexCursor()->line(), m_hexdocument->lastColumn());
            else this->hexCursor()->move(this->hexCursor()->line(), this->options().linelength - 1);
        }
        else
        {
            if(this->hexCursor()->line() == m_hexdocument->lastLine()) this->hexCursor()->select(this->hexCursor()->line(), m_hexdocument->lastColumn()); else
                this->hexCursor()->select(this->hexCursor()->line(), this->options().linelength - 1);
        }
    }
    else
        return false;

    return true;
}

bool QHexView::keyPressTextInput(QKeyEvent* e)
{
    if(m_readonly || e->text().isEmpty() || (e->modifiers() & Qt::ControlModifier)) return false;

    auto* cursor = m_hexdocument->cursor();
    bool atend = cursor->offset() >= m_hexdocument->length();
    if(atend && cursor->mode() == QHexCursor::Mode::Overwrite) return false;

    auto key = static_cast<char>(e->text().at(0).toLatin1());

    switch(m_currentarea)
    {
        case Area::Hex: {
            if(!std::isxdigit(key)) return false;

            bool ok = false;
            auto val = static_cast<uchar>(QString(key).toUInt(&ok, 16));
            if(!ok) return false;
            cursor->removeSelection();

            uchar ch = m_hexdocument->at(cursor->offset());
            ch = m_writing ? (ch << 4) | val : val;

            if(!m_writing && (cursor->mode() == QHexCursor::Mode::Insert)) m_hexdocument->insert(cursor->offset(), val);
            else m_hexdocument->replace(cursor->offset(), ch);

            m_writing = !m_writing;
            if(!m_writing) this->moveNext();
            break;
        }

        case Area::Ascii: {
            if(!std::isprint(key)) return false;
            cursor->removeSelection();
            if(cursor->mode() == QHexCursor::Mode::Insert) m_hexdocument->insert(cursor->offset(), key);
            else m_hexdocument->replace(cursor->offset(), key);
            this->moveNext();
            break;
        }

        default: return false;
    }

    return true;
}

bool QHexView::keyPressAction(QKeyEvent* e)
{
   if(e->modifiers() != Qt::NoModifier)
   {
       if(e->matches(QKeySequence::SelectAll))
       {
           m_hexdocument->cursor()->move(0, 0);
           m_hexdocument->cursor()->select(m_hexdocument->lastLine(), m_hexdocument->lastColumn() - 1);
       }
       else if(!m_readonly && e->matches(QKeySequence::Undo)) m_hexdocument->undo();
       else if(!m_readonly && e->matches(QKeySequence::Redo)) m_hexdocument->redo();
       else if(!m_readonly && e->matches(QKeySequence::Cut)) m_hexdocument->cut(m_currentarea == Area::Hex);
       else if(e->matches(QKeySequence::Copy)) m_hexdocument->copy(m_currentarea == Area::Hex);
       else if(!m_readonly && e->matches(QKeySequence::Paste)) m_hexdocument->paste(m_currentarea == Area::Hex);
       else return false;

       return true;
   }

   if(m_readonly) return false;

   switch(e->key())
   {
       case Qt::Key_Backspace:
       case Qt::Key_Delete: {
           if(!m_hexdocument->cursor()->hasSelection()) {
               auto offset = m_hexdocument->cursor()->offset();
               if(offset <= 0) return true;

               if(e->key() == Qt::Key_Backspace) m_hexdocument->remove(offset - 1, 1);
               else m_hexdocument->remove(offset, 1);
           }
           else
           {
               auto oldpos = m_hexdocument->cursor()->selectionStart();
               m_hexdocument->cursor()->removeSelection();
               m_hexdocument->cursor()->move(oldpos);
           }

           if(e->key() == Qt::Key_Backspace) this->movePrevious();
           break;
       }

       case Qt::Key_Insert: m_hexdocument->cursor()->toggleMode(); break;
       default: return false;
   }

   return true;
}

bool QHexView::event(QEvent* e)
{
    if(e->type() == QEvent::FontChange)
    {
        m_fontmetrics = QFontMetricsF(this->font());
        return true;
    }

    return QAbstractScrollArea::event(e);
}

void QHexView::paintEvent(QPaintEvent*)
{
    m_textdocument.clear();
    m_textdocument.setDefaultFont(this->font());

    QPainter painter(this->viewport());
    QTextCursor c(&m_textdocument);
    this->drawHeader(c);
    this->drawDocument(c);

    m_textdocument.drawContents(&painter);
}

void QHexView::resizeEvent(QResizeEvent* e)
{
    this->checkState();
    QAbstractScrollArea::resizeEvent(e);
}

void QHexView::mousePressEvent(QMouseEvent* e)
{
    QAbstractScrollArea::mousePressEvent(e);
    if(e->button() != Qt::LeftButton) return;

    auto pos = this->positionFromPoint(e->pos());
    if(!pos.isValid()) return;

    auto area = this->areaFromPoint(e->pos());

    switch(area)
    {
        case Area::Address: this->hexCursor()->move(pos.line, 0); break;
        case Area::Hex: m_currentarea = area; this->hexCursor()->move(pos); break;
        case Area::Ascii: m_currentarea = area; this->hexCursor()->move(pos.line, pos.column); break;
        default: return;
    }

    this->viewport()->update();
}

void QHexView::mouseMoveEvent(QMouseEvent* e)
{
    QAbstractScrollArea::mouseMoveEvent(e);
    if(!this->hexCursor()) return;

    e->accept();
    auto area = this->areaFromPoint(e->pos());

    switch(area)
    {
        case Area::Header: this->viewport()->setCursor(Qt::ArrowCursor); return;
        case Area::Address: this->viewport()->setCursor(Qt::ArrowCursor); break;
        default: this->viewport()->setCursor(Qt::IBeamCursor); break;
    }

    if(e->buttons() == Qt::LeftButton)
    {
        auto pos = this->positionFromPoint(e->pos());
        if(!pos.isValid()) return;
        if(area == Area::Ascii || area == Area::Hex) m_currentarea = area;
        this->hexCursor()->select(pos);
        this->viewport()->update();
    }
}

void QHexView::keyPressEvent(QKeyEvent* e)
{
    bool handled = false;

    if(this->hexCursor())
    {
        handled = this->keyPressMove(e);
        if(!handled) handled = this->keyPressAction(e);
        if(!handled) handled = this->keyPressTextInput(e);
    }

    if(handled) e->accept();
    else QAbstractScrollArea::keyPressEvent(e);
}
