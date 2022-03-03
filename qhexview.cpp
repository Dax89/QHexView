#include "qhexview.h"
#include "model/qhexcursor.h"
#include "model/qhexutils.h"
#include <QMouseEvent>
#include <QFontDatabase>
#include <QApplication>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollBar>
#include <QToolTip>
#include <QPalette>
#include <QPainter>
#include <climits>
#include <cctype>
#include <cmath>

#if defined(QHEXVIEW_DEBUG)
    #include <QMetaEnum>
    #include <QDebug>

    #define qhexview_fmtprint(fmt, ...) qDebug("%s " fmt, __func__, __VA_ARGS__)
    #define qhexview_enumname(val) QMetaEnum::fromType<decltype(val)>().valueToKey(static_cast<int>(val))
#else
    #define qhexview_fmtprint(fmt, ...)
    #define qhexview_enumname(val)
#endif

QHexView::QHexView(QWidget *parent) : QAbstractScrollArea(parent), m_fontmetrics(this->font())
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    if(f.styleHint() != QFont::TypeWriter)
    {
        f.setFamily("Monospace"); // Force Monospaced font
        f.setStyleHint(QFont::TypeWriter);
    }

    m_textdocument.setDocumentMargin(0);

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
QHexCursor* QHexView::hexCursor() const { return m_hexdocument ? m_hexdocument->cursor() : nullptr; }
const QHexOptions* QHexView::options() const { return m_hexdocument ? m_hexdocument->options() : nullptr; }
void QHexView::setOptions(const QHexOptions& options) { if(m_hexdocument) m_hexdocument->setOptions(options); }

void QHexView::setDocument(QHexDocument* doc)
{
    m_writing = false;

    if(m_hexdocument)
    {
        disconnect(m_hexdocument->metadata(), &QHexMetadata::changed, this, nullptr);
        disconnect(m_hexdocument->cursor(), &QHexCursor::positionChanged, this, nullptr);
        disconnect(m_hexdocument->cursor(), &QHexCursor::modeChanged, this, nullptr);
        disconnect(m_hexdocument, &QHexDocument::changed, this, nullptr);
    }

    m_hexdocument = doc;
    connect(m_hexdocument, &QHexDocument::changed, this, [=]() { this->checkAndUpdate(true); });
    connect(m_hexdocument->cursor(), &QHexCursor::positionChanged, this, [=]() { m_writing = false; this->viewport()->update(); });
    connect(m_hexdocument->cursor(), &QHexCursor::modeChanged, this, [=]() { m_writing = false; this->viewport()->update(); });
    connect(m_hexdocument->metadata(), &QHexMetadata::changed, this, [=]() { this->viewport()->update(); });
    this->checkAndUpdate(true);
}

void QHexView::setByteColor(quint8 b, QHexColor c) { if(m_hexdocument) m_hexdocument->setByteColor(b, c); }
void QHexView::setByteForeground(quint8 b, QColor c) { if(m_hexdocument) m_hexdocument->setByteForeground(b, c); }
void QHexView::setByteBackground(quint8 b, QColor c) { if(m_hexdocument) m_hexdocument->setByteBackground(b, c); }
void QHexView::setMetadata(qint64 begin, qint64 end, const QColor& fgcolor, const QColor& bgcolor, const QString& comment) { if(m_hexdocument) m_hexdocument->metadata()->setMetadata(begin, end, fgcolor, bgcolor, comment); }
void QHexView::setForeground(qint64 begin, qint64 end, const QColor& fgcolor) { if(m_hexdocument) m_hexdocument->metadata()->setForeground(begin, end, fgcolor); }
void QHexView::setBackground(qint64 begin, qint64 end, const QColor& bgcolor) { if(m_hexdocument) m_hexdocument->metadata()->setBackground(begin, end, bgcolor); }
void QHexView::setComment(qint64 begin, qint64 end, const QString& comment) { if(m_hexdocument) m_hexdocument->metadata()->setComment(begin, end, comment); }
void QHexView::setMetadataSize(qint64 begin, qint64 length, const QColor& fgcolor, const QColor& bgcolor, const QString& comment) { if(m_hexdocument) m_hexdocument->metadata()->setMetadataSize(begin, length, fgcolor, bgcolor, comment); }
void QHexView::setForegroundSize(qint64 begin, qint64 length, const QColor& fgcolor) { if(m_hexdocument) m_hexdocument->metadata()->setForegroundSize(begin, length, fgcolor); }
void QHexView::setBackgroundSize(qint64 begin, qint64 length, const QColor& bgcolor) { if(m_hexdocument) m_hexdocument->metadata()->setBackgroundSize(begin, length, bgcolor); }
void QHexView::setCommentSize(qint64 begin, qint64 length, const QString& comment) { if(m_hexdocument) m_hexdocument->metadata()->setCommentSize(begin, length, comment); }
void QHexView::removeMetadata(qint64 line) { if(m_hexdocument) m_hexdocument->metadata()->removeMetadata(line); }
void QHexView::removeBackground(qint64 line) { if(m_hexdocument) m_hexdocument->metadata()->removeBackground(line); }
void QHexView::removeForeground(qint64 line) { if(m_hexdocument) m_hexdocument->metadata()->removeForeground(line); }
void QHexView::removeComments(qint64 line) { if(m_hexdocument) m_hexdocument->metadata()->removeComments(line); }
void QHexView::unhighlight(qint64 line) { if(m_hexdocument) m_hexdocument->metadata()->unhighlight(line); }
void QHexView::clearMetadata() { if(m_hexdocument) m_hexdocument->metadata()->clear(); }
void QHexView::setScrollSteps(unsigned int l) { if(m_hexdocument) m_hexdocument->setScrollSteps(l); }
void QHexView::setReadOnly(bool r) { m_readonly = r; }
void QHexView::setLineLength(unsigned int l) { m_hexdocument->setLineLength(l); this->checkAndUpdate(true); }
void QHexView::setGroupLength(unsigned int l) { m_hexdocument->setGroupLength(l); }

void QHexView::checkState()
{
    if(!m_hexdocument) return;
    m_hexdocument->checkOptions(this->palette());

    this->verticalScrollBar()->setSingleStep(this->options()->scrollsteps);
    this->verticalScrollBar()->setPageStep(this->options()->scrollsteps);

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

    this->setHorizontalScrollBarPolicy(this->viewport()->width() <= this->endColumnX() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    this->horizontalScrollBar()->setMaximum(this->endColumnX());
}

void QHexView::checkAndUpdate(bool calccolumns)
{
    this->checkState();
    if(calccolumns) this->calcColumns();
    this->viewport()->update();
}

void QHexView::calcColumns()
{
    if(!m_hexdocument) return;

    m_hexcolumns.clear();
    m_hexcolumns.reserve(this->options()->linelength);

    auto x = this->hexColumnX(), cw = this->cellWidth() * 2;

    for(auto i = 0u; i < this->options()->linelength; i++)
    {
        for(auto j = 0u; j < this->options()->grouplength; j++, x += cw)
            m_hexcolumns.push_back(QRect(x, 0, cw, 0));

        x += this->cellWidth();
    }
}

void QHexView::drawSeparators(QPainter* p) const
{
    if(!this->options()->separators) return;

    const auto extramargin = this->cellWidth() / 2.0;

    p->save();
        p->setPen(this->options()->separatorcolor.isValid() ? this->options()->separatorcolor : this->palette().color(QPalette::Dark));
        p->drawLine(QLineF(0, m_fontmetrics.lineSpacing(), this->endColumnX() - extramargin, m_fontmetrics.lineSpacing()));
        p->drawLine(QLineF(this->hexColumnX() - extramargin, 0, this->hexColumnX() - extramargin, this->height()));
        p->drawLine(QLineF(this->asciiColumnX() - extramargin, 0, this->asciiColumnX() - extramargin, this->height()));
        p->drawLine(QLineF(this->endColumnX() - extramargin, 0, this->endColumnX() - extramargin, this->height()));
    p->restore();
}

void QHexView::renderHeader(QTextCursor& c) const
{
    if(!this->options()->header) return;

    QString addressheader = this->options()->addresslabel.rightJustified(this->addressWidth()), hexheader;

    for(auto i = 0u; i < this->options()->linelength; i += this->options()->grouplength)
        hexheader.append(QString("%1 ").arg(QString::number(i, 16).rightJustified(this->options()->grouplength * 2, '0')).toUpper());

    QTextCharFormat cf;
    cf.setForeground(this->options()->headercolor);

    c.insertText(m_fontmetrics.elidedText(addressheader, Qt::ElideRight, this->hexColumnX()) + " ", cf);
    c.insertText(hexheader, cf);
    c.insertText(this->options()->asciilabel, cf);

    c.insertBlock();
}

void QHexView::renderDocument(QTextCursor& c) const
{
    if(!m_hexdocument) return;

    qreal y = this->options()->header ? this->lineHeight() : 0;
    quint64 line = static_cast<quint64>(this->verticalScrollBar()->value());

    QTextCharFormat addrformat;
    addrformat.setForeground(this->palette().color(QPalette::Normal, QPalette::Highlight));

    for(qint64 l = 0; m_hexdocument->isEmpty() || (line < m_hexdocument->lines() && l < this->visibleLines()); l++, line++, y += this->lineHeight())
    {
        quint64 address = line * this->options()->linelength + m_hexdocument->baseAddress();
        QString addrstr = QString::number(address, 16).rightJustified(this->addressWidth(), '0').toUpper();

        // Address Part
        QTextCharFormat acf;
        acf.setForeground(this->options()->headercolor);
        c.insertText(addrstr + " ", acf);

        auto linebytes = m_hexdocument->getLine(line);

        // Hex Part
        for(auto column = 0u; column < this->options()->linelength; )
        {
            QTextCharFormat cf;

            for(auto byteidx = 0u; byteidx < this->options()->grouplength; byteidx++, column++)
            {
                QString s = linebytes.isEmpty() || column >= static_cast<qint64>(linebytes.size()) ? "  " : QString(QHexUtils::toHex(linebytes.mid(column, 1)).toUpper());
                quint8 b = static_cast<int>(column) < linebytes.size() ? linebytes.at(column) : 0x00;
                cf = this->drawFormat(c, b, s, Area::Hex, line, column, static_cast<int>(column) < linebytes.size());
            }

            c.insertText(" ", cf);
        }

        // Ascii Part
        for(auto column = 0u; column < this->options()->linelength; column++)
        {
            auto s = linebytes.isEmpty() ||
                     column >= static_cast<qint64>(linebytes.size()) ? QChar(' ') :
                                                                       (std::isprint(static_cast<quint8>(linebytes.at(column))) ? QChar(linebytes.at(column)) : this->options()->unprintablechar);

            quint8 b = static_cast<int>(column) < linebytes.size() ? linebytes.at(column) : 0x00;
            this->drawFormat(c, b, s, Area::Ascii, line, column, static_cast<int>(column) < linebytes.size());
        }

        QTextBlockFormat bf;

        if(this->options()->linealternatebackground.isValid() && line % 2)
        {
            bf.setBackground(this->options()->linealternatebackground);
            bf.setForeground(QHexView::getTextColor(bf.background().color()));
        }
        else if(this->options()->linebackground.isValid() && !(line % 2))
        {
            bf.setBackground(this->options()->linebackground);
            bf.setForeground(QHexView::getTextColor(bf.background().color()));
        }

        c.insertBlock(bf);
        if(m_hexdocument->isEmpty()) break;
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
    if(this->options()->header) vl--;
    return static_cast<int>(std::min(m_hexdocument->lines(), vl));
}

qreal QHexView::hexColumnWidth() const
{
    int l = 0;

    for(auto i = 0u; i < this->options()->linelength; i += this->options()->grouplength)
        l += (1 << this->options()->grouplength) + 1;

    return this->getNCellsWidth(l);
}

unsigned int QHexView::addressWidth() const { return 8; }
qreal QHexView::hexColumnX() const { return this->getNCellsWidth(this->addressWidth()) + this->cellWidth(); }
qreal QHexView::asciiColumnX() const { return this->hexColumnX() + this->hexColumnWidth(); }
qreal QHexView::endColumnX() const { return this->asciiColumnX() + this->cellWidth() + this->getNCellsWidth(this->options()->linelength); }
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
    auto abspt = this->absolutePoint(pt);

    switch(this->areaFromPoint(pt))
    {
        case Area::Hex: {
            pos.column = -1;

            for(qint64 i = 0; i < m_hexcolumns.size(); i++) {
                if(m_hexcolumns.at(i).left() > abspt.x()) break;
                pos.column = i;
            }

            break;
        }

        case Area::Ascii: pos.column = std::floor((abspt.x() - this->asciiColumnX()) / this->cellWidth()); break;
        case Area::Address: pos.column = 0; break;
        case Area::Header: return QHexCursor::Position::invalid();
        default: break;
    }

    pos.line = std::min<qint64>(this->verticalScrollBar()->value() + (abspt.y() / this->lineHeight()), m_hexdocument->lines());
    if(this->options()->header) pos.line = std::max<qint64>(0, pos.line - 1);

    auto docline = m_hexdocument->getLine(pos.line);
    pos.column = std::min<qint64>(pos.column, docline.isEmpty() ? 0 : docline.size());

    qhexview_fmtprint("line: %lld, col: %lld", pos.line, pos.column);
    return pos;
}

QPoint QHexView::absolutePoint(QPoint pt) const { return pt + QPoint(this->horizontalScrollBar()->value(), 0); }

QHexView::Area QHexView::areaFromPoint(QPoint pt) const
{
    pt = this->absolutePoint(pt);
    qreal line = this->verticalScrollBar()->value() + pt.y() / this->lineHeight();

    if(this->options()->header && !std::floor(line)) return Area::Header;
    if(pt.x() < this->hexColumnX()) return Area::Address;
    if(pt.x() < this->asciiColumnX()) return Area::Hex;
    if(pt.x() < this->endColumnX()) return Area::Ascii;
    return Area::Extra;
}

QTextCharFormat QHexView::drawFormat(QTextCursor& c, quint8 b, const QString& s, Area area, qint64 line, qint64 column, bool applyformat) const
{
    QTextCharFormat cf, selcf;
    const auto& options = m_hexdocument->options();

    if(applyformat)
    {
        auto it = options->bytecolors.find(b);

        if(it != options->bytecolors.end())
        {
            if(it->background.isValid()) cf.setBackground(it->background);
            if(it->foreground.isValid()) cf.setForeground(it->foreground);
        }

        const auto* metadataline = m_hexdocument->metadata()->find(line);

        if(metadataline)
        {
            auto offset = m_hexdocument->cursor()->positionToOffset({line, column});

            for(const auto& metadata : *metadataline)
            {
                if(offset < metadata.begin || offset >= metadata.end) continue;
                if(metadata.background.isValid()) cf.setBackground(metadata.background);
                if(metadata.foreground.isValid()) cf.setForeground(metadata.foreground);
                if(column < m_hexdocument->getLastColumn(line)) selcf = cf;

                if(!metadata.comment.isEmpty())
                {
                    if(m_hexdocument->options()->commentcolor.isValid())
                        cf.setUnderlineColor(m_hexdocument->options()->commentcolor);

                    cf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::SingleUnderline);
                }
            }
        }
    }

    if(this->hexCursor()->isSelected(line, column))
    {
        cf.setBackground(this->palette().color(QPalette::Normal, QPalette::Highlight));
        cf.setForeground(this->palette().color(QPalette::Normal, QPalette::HighlightedText));
        if(column < m_hexdocument->getLastColumn(line)) selcf = cf;
    }

    if(this->hasFocus() && this->hexCursor()->line() == line && this->hexCursor()->column() == column)
    {
        auto cursorbg = this->palette().color(QPalette::Normal, QPalette::WindowText);
        auto cursorfg = this->palette().color(QPalette::Normal, QPalette::Window);
        auto discursorbg = this->palette().color(QPalette::Disabled, QPalette::WindowText);
        auto discursorfg = this->palette().color(QPalette::Disabled, QPalette::Window);

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

    if(column >= this->options()->linelength - 1)
    {
        line++;
        column = 0;
    }
    else
        column++;

    const qint64 offset = this->hexCursor()->mode() == QHexCursor::Mode::Insert ? 1 : 0;
    if(select) this->hexCursor()->select(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->getLastColumn(line) + offset));
    else this->hexCursor()->move(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->getLastColumn(line) + offset));
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

    if(select) this->hexCursor()->select(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->getLastColumn(line)));
    else this->hexCursor()->move(std::min<qint64>(line, m_hexdocument->lines()), std::min<qint64>(column, m_hexdocument->getLastColumn(line)));
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
        if(e->matches(QKeySequence::MoveToEndOfDocument)) this->hexCursor()->move(m_hexdocument->lastLine(), m_hexdocument->getLastColumn(this->hexCursor()->line()));
        else this->hexCursor()->select(m_hexdocument->lastLine(), m_hexdocument->getLastColumn(m_hexdocument->lastLine()));
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
            if(this->hexCursor()->line() == m_hexdocument->lastLine()) this->hexCursor()->move(this->hexCursor()->line(), m_hexdocument->getLastColumn(this->hexCursor()->line()));
            else this->hexCursor()->move(this->hexCursor()->line(), this->options()->linelength - 1);
        }
        else
        {
            if(this->hexCursor()->line() == m_hexdocument->lastLine()) this->hexCursor()->select(this->hexCursor()->line(), m_hexdocument->getLastColumn(this->hexCursor()->line())); else
                this->hexCursor()->select(this->hexCursor()->line(), this->options()->linelength - 1);
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

    auto key = static_cast<quint8>(e->text().at(0).toLatin1());

    switch(m_currentarea)
    {
        case Area::Hex: {
            if(!std::isxdigit(key)) return false;

            bool ok = false;
            auto val = static_cast<quint8>(QString(key).toUInt(&ok, 16));
            if(!ok) return false;
            cursor->removeSelection();

            quint8 ch = m_hexdocument->isEmpty() || cursor->offset() >= m_hexdocument->length() ? '\x00' : m_hexdocument->at(cursor->offset());
            ch = m_writing ? (ch << 4) | val : val;

            if(!m_writing && (cursor->mode() == QHexCursor::Mode::Insert)) m_hexdocument->insert(cursor->offset(), val);
            else m_hexdocument->replace(cursor->offset(), ch);

            m_writing = !m_writing;
            if(!m_writing)
                this->moveNext();

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
           m_hexdocument->cursor()->select(m_hexdocument->lastLine(), m_hexdocument->getLastColumn(m_hexdocument->cursor()->line()));
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
    switch(e->type())
    {
        case QEvent::FontChange:
            m_fontmetrics = QFontMetricsF(this->font());
            this->checkAndUpdate(true);
            return true;

        case QEvent::ToolTip: {
            if(m_hexdocument && (m_currentarea == Area::Hex || m_currentarea == Area::Ascii)) {
                auto* helpevent = static_cast<QHelpEvent*>(e);
                auto pos = this->positionFromPoint(helpevent->pos());
                auto comment = m_hexdocument->metadata()->getComment(pos.line, pos.column);
                if(!comment.isEmpty()) QToolTip::showText(helpevent->globalPos(), comment);
                return true;
            }

            break;
        }

        default: break;
    }

    return QAbstractScrollArea::event(e);
}

void QHexView::paintEvent(QPaintEvent*)
{
    if(!m_hexdocument) return;

    m_textdocument.clear();
    m_textdocument.setDefaultFont(this->font());
    QTextCursor c(&m_textdocument);

    QPainter painter(this->viewport());
    painter.setFont(this->font());
    this->renderHeader(c);
    this->renderDocument(c);

    painter.save();
        painter.translate(-this->horizontalScrollBar()->value(), 0);
        m_textdocument.drawContents(&painter);
    this->drawSeparators(&painter);
    painter.restore();

}

void QHexView::resizeEvent(QResizeEvent* e)
{
    this->checkState();
    QAbstractScrollArea::resizeEvent(e);
}

void QHexView::focusInEvent(QFocusEvent* e)
{
    QAbstractScrollArea::focusInEvent(e);
    if(m_hexdocument) this->viewport()->update();
}

void QHexView::focusOutEvent(QFocusEvent* e)
{
    QAbstractScrollArea::focusOutEvent(e);
    if(m_hexdocument) this->viewport()->update();
}

void QHexView::mousePressEvent(QMouseEvent* e)
{
    QAbstractScrollArea::mousePressEvent(e);
    if(!m_hexdocument || e->button() != Qt::LeftButton) return;

    auto pos = this->positionFromPoint(e->pos());
    if(!pos.isValid()) return;

    auto area = this->areaFromPoint(e->pos());
    qhexview_fmtprint("%s", qhexview_enumname(area));

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

bool QHexView::isColorLight(QColor c)
{
    return std::sqrt(0.299 * std::pow(c.red(), 2) +
                     0.587 * std::pow(c.green(), 2) +
                     0.114 * std::pow(c.blue(), 2)) > 127.5;
}

QColor QHexView::getTextColor(QColor c) { return QHexView::isColorLight(c) ? Qt::white : Qt::black; }
