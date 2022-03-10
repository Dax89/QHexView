#include "qhexview.h"
#include "model/buffer/qmemorybuffer.h"
#include "model/qhexcursor.h"
#include "model/qhexutils.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTextDocument>
#include <QFontDatabase>
#include <QApplication>
#include <QClipboard>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollBar>
#include <QToolTip>
#include <QPalette>
#include <QPainter>
#include <limits>
#include <cctype>
#include <cmath>

#if defined(QHEXVIEW_DEBUG)
    #include <QDebug>
    #define qhexview_fmtprint(fmt, ...) qDebug("%s " fmt, __func__, __VA_ARGS__)
#else
    #define qhexview_fmtprint(fmt, ...)
#endif

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

    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int) { this->viewport()->update(); });

    m_hexmetadata = new QHexMetadata(&m_options, this);
    connect(m_hexmetadata, &QHexMetadata::changed, this, [=]() { this->viewport()->update(); });

    m_hexcursor = new QHexCursor(&m_options, this);
    this->setDocument(QHexDocument::fromMemory<QMemoryBuffer>(QByteArray(), this));
    this->checkState();

    connect(m_hexcursor, &QHexCursor::positionChanged, this, &QHexView::positionChanged);
    connect(m_hexcursor, &QHexCursor::modeChanged, this, &QHexView::modeChanged);
}

QHexDocument* QHexView::hexDocument() const { return m_hexdocument; }
QHexCursor* QHexView::hexCursor() const { return m_hexdocument ? m_hexcursor : nullptr; }
const QHexMetadata* QHexView::hexMetadata() const { return m_hexmetadata; }
QHexOptions QHexView::options() const { return m_options; }

void QHexView::setOptions(const QHexOptions& options)
{
    auto oldlinelength = m_options.linelength;
    m_options = options;

    if(oldlinelength != m_options.linelength)
        m_hexmetadata->invalidate();

    this->checkAndUpdate();
}

void QHexView::setBaseAddress(quint64 baseaddress)
{
    if(m_options.baseaddress == baseaddress) return;

    m_options.baseaddress = baseaddress;
    this->checkAndUpdate();
}

void QHexView::setDelegate(QHexDelegate* rd)
{
    if(m_hexdelegate == rd) return;
    m_hexdelegate = rd;
    this->checkAndUpdate();
}

void QHexView::setDocument(QHexDocument* doc)
{
    if(!doc) doc = QHexDocument::fromMemory<QMemoryBuffer>(QByteArray(), this);

    m_writing = false;
    m_hexmetadata->clear();
    m_hexcursor->move(0);

    if(m_hexdocument)
    {
        disconnect(m_hexcursor, &QHexCursor::positionChanged, this, nullptr);
        disconnect(m_hexcursor, &QHexCursor::modeChanged, this, nullptr);
        disconnect(m_hexdocument, &QHexDocument::changed, this, nullptr);
        disconnect(m_hexdocument, &QHexDocument::dataChanged, this, nullptr);
        disconnect(m_hexdocument, &QHexDocument::reset, this, nullptr);
    }

    m_hexdocument = doc;

    connect(m_hexdocument, &QHexDocument::reset, this, [=]() {
        m_writing = false;
        m_hexcursor->move(0);
        this->checkAndUpdate(true);
    });

    connect(m_hexdocument, &QHexDocument::dataChanged, this, &QHexView::dataChanged);
    connect(m_hexdocument, &QHexDocument::changed, this, [=]() { this->checkAndUpdate(true); });
    connect(m_hexcursor, &QHexCursor::positionChanged, this, [=]() { m_writing = false; this->ensureVisible(); });
    connect(m_hexcursor, &QHexCursor::modeChanged, this, [=]() { m_writing = false; this->viewport()->update(); });
    this->checkAndUpdate(true);
}

void QHexView::setData(const QByteArray& ba) { m_hexdocument->setData(ba); }
void QHexView::setData(QHexBuffer* buffer) { m_hexdocument->setData(buffer); }
void QHexView::setCursorMode(QHexCursor::Mode mode) { m_hexcursor->setMode(mode); }

void QHexView::setByteColor(quint8 b, QHexColor c)
{
    m_options.bytecolors[b] = c;
    this->checkAndUpdate();
}

void QHexView::setByteForeground(quint8 b, QColor c)
{
    m_options.bytecolors[b].foreground = c;
    this->checkAndUpdate();
}

void QHexView::setByteBackground(quint8 b, QColor c)
{
    m_options.bytecolors[b].background = c;
    this->checkAndUpdate();
}

void QHexView::setMetadata(qint64 begin, qint64 end, const QColor& fgcolor, const QColor& bgcolor, const QString& comment) { m_hexmetadata->setMetadata(begin, end, fgcolor, bgcolor, comment); }
void QHexView::setForeground(qint64 begin, qint64 end, const QColor& fgcolor) { m_hexmetadata->setForeground(begin, end, fgcolor); }
void QHexView::setBackground(qint64 begin, qint64 end, const QColor& bgcolor) { m_hexmetadata->setBackground(begin, end, bgcolor); }
void QHexView::setComment(qint64 begin, qint64 end, const QString& comment) { m_hexmetadata->setComment(begin, end, comment); }
void QHexView::setMetadataSize(qint64 begin, qint64 length, const QColor& fgcolor, const QColor& bgcolor, const QString& comment) { m_hexmetadata->setMetadataSize(begin, length, fgcolor, bgcolor, comment); }
void QHexView::setForegroundSize(qint64 begin, qint64 length, const QColor& fgcolor) { m_hexmetadata->setForegroundSize(begin, length, fgcolor); }
void QHexView::setBackgroundSize(qint64 begin, qint64 length, const QColor& bgcolor) { m_hexmetadata->setBackgroundSize(begin, length, bgcolor); }
void QHexView::setCommentSize(qint64 begin, qint64 length, const QString& comment) { m_hexmetadata->setCommentSize(begin, length, comment); }
void QHexView::removeMetadata(qint64 line) { m_hexmetadata->removeMetadata(line); }
void QHexView::removeBackground(qint64 line) { m_hexmetadata->removeBackground(line); }
void QHexView::removeForeground(qint64 line) { m_hexmetadata->removeForeground(line); }
void QHexView::removeComments(qint64 line) { m_hexmetadata->removeComments(line); }
void QHexView::unhighlight(qint64 line) { m_hexmetadata->unhighlight(line); }
void QHexView::clearMetadata() { m_hexmetadata->clear(); }
void QHexView::undo() { if(m_hexdocument) m_hexdocument->undo(); }
void QHexView::redo() { if(m_hexdocument) m_hexdocument->redo(); }

void QHexView::cut(bool hex)
{
    if(!m_hexcursor->hasSelection()) return;
    this->copy(hex);
    m_hexcursor->removeSelection();
}

void QHexView::copy(bool hex) const
{
    if(!m_hexcursor->hasSelection()) return;

    QClipboard* c = qApp->clipboard();
    QByteArray bytes = m_hexcursor->selectedBytes();
    if(hex) bytes = QHexUtils::toHex(bytes, ' ').toUpper();
    c->setText(bytes);
}

void QHexView::paste(bool hex)
{
    QClipboard* c = qApp->clipboard();
    QByteArray pastedata = c->text().toUtf8();
    if(pastedata.isEmpty()) return;

    m_hexcursor->removeSelection();
    if(hex) pastedata = QByteArray::fromHex(pastedata);

    switch(m_hexcursor->mode())
    {
        case QHexCursor::Mode::Insert: m_hexdocument->insert(m_hexcursor->offset(), pastedata); break;
        default: m_hexdocument->replace(m_hexcursor->offset(), pastedata); break;
    }
}

void QHexView::selectAll()
{
    m_hexcursor->move(0);
    m_hexcursor->select(m_hexdocument->length());
}

void QHexView::removeSelection()
{
    if(!m_hexcursor->hasSelection()) return;
    m_hexdocument->remove(m_hexcursor->selectionStartOffset(), m_hexcursor->selectionLength() - 1);
    m_hexcursor->clearSelection();
}

void QHexView::switchMode() { m_hexcursor->switchMode(); }

void QHexView::setAddressWidth(unsigned int w)
{
    if(w == m_options.addresswidth) return;
    m_options.addresswidth = w;
    this->checkState();
}

void QHexView::setScrollSteps(unsigned int l)
{
    if(l == m_options.scrollsteps) return;
    m_options.scrollsteps = std::max(1u, l);
}

void QHexView::setReadOnly(bool r) { m_readonly = r; }

void QHexView::setAutoWidth(bool r)
{
    if(m_autowidth == r) return;
    m_autowidth = r;
    this->checkState();
}

void QHexView::checkOptions()
{
    if(m_options.grouplength > m_options.linelength) m_options.grouplength = m_options.linelength;
    if(!m_options.scrollsteps) m_options.scrollsteps = 1;

    m_options.addresswidth = std::max<unsigned int>(m_options.addresswidth, this->calcAddressWidth());

    // Round to nearest multiple of 2
    m_options.grouplength = 1u << (static_cast<unsigned int>(std::floor(m_options.grouplength / 2.0)));

    if(m_options.grouplength <= 1) m_options.grouplength = 1;

    if(!m_options.headercolor.isValid())
        m_options.headercolor = this->palette().color(QPalette::Normal, QPalette::Highlight);
}

void QHexView::setLineLength(unsigned int l)
{
    if(l == m_options.linelength) return;
    m_options.linelength = l;
    m_hexmetadata->invalidate();
    this->checkAndUpdate(true);
}

void QHexView::setGroupLength(unsigned int l)
{
    if(l == m_options.grouplength) return;
    m_options.grouplength = l;
    this->checkAndUpdate(true);
}

void QHexView::checkState()
{
    if(!m_hexdocument) return;
    this->checkOptions();

    int doclines = static_cast<int>(this->lines()), vislines = this->visibleLines(true);
    qint64 vscrollmax = doclines - vislines;
    if(doclines >= vislines) vscrollmax++;

    this->verticalScrollBar()->setRange(0, std::max<qint64>(0, vscrollmax));
    this->verticalScrollBar()->setPageStep(vislines - 1);
    this->verticalScrollBar()->setSingleStep(m_options.scrollsteps);
    this->setVerticalScrollBarPolicy(doclines < this->visibleLines(true) ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAlwaysOn);

    int vw = this->verticalScrollBar()->isVisible() ? this->verticalScrollBar()->width() : 0;

    static int oldmw = 0;
    if(!oldmw) oldmw = this->maximumWidth();
    this->setMaximumWidth(m_autowidth ? this->endColumnX() + vw : oldmw);

    this->horizontalScrollBar()->setRange(0, std::max<int>(0, this->endColumnX() - this->width() + vw));
    this->horizontalScrollBar()->setPageStep(this->width());
    this->setHorizontalScrollBarPolicy(this->width() < static_cast<int>(std::floor(this->endColumnX())) ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
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
    m_hexcolumns.reserve(m_options.linelength);

    auto x = this->hexColumnX(), cw = this->cellWidth() * 2;

    for(auto i = 0u; i < m_options.linelength; i++)
    {
        for(auto j = 0u; j < m_options.grouplength; j++, x += cw)
            m_hexcolumns.push_back(QRect(x, 0, cw, 0));

        x += this->cellWidth();
    }
}

void QHexView::ensureVisible()
{
    if(!m_hexdocument) return;

    auto pos = m_hexcursor->position();
    auto vlines = this->visibleLines();

    if(pos.line >= (this->verticalScrollBar()->value() + vlines))
        this->verticalScrollBar()->setValue(pos.line - vlines + 1);
    else if(pos.line < this->verticalScrollBar()->value())
        this->verticalScrollBar()->setValue(pos.line);
    else
        this->viewport()->update();
}

void QHexView::drawSeparators(QPainter* p) const
{
    if(!m_options.hasFlag(QHexFlags::Separators)) return;

    auto oldpen = p->pen();
    p->setPen(m_options.separatorcolor.isValid() ? m_options.separatorcolor : this->palette().color(QPalette::Dark));

    if(m_options.hasFlag(QHexFlags::HSeparator))
        p->drawLine(QLineF(0, m_fontmetrics.lineSpacing(), this->endColumnX(), m_fontmetrics.lineSpacing()));

    if(m_options.hasFlag(QHexFlags::VSeparator))
    {
        p->drawLine(QLineF(this->hexColumnX(), 0, this->hexColumnX(), this->height()));
        p->drawLine(QLineF(this->asciiColumnX(), 0, this->asciiColumnX(), this->height()));
    }

    p->setPen(oldpen);
}

void QHexView::drawHeader(QTextCursor& c) const
{
    if(m_options.hasFlag(QHexFlags::NoHeader)) return;

    static const auto RESET_FORMAT = [](const QHexOptions& options, QTextCharFormat& cf) {
        cf = { };
        cf.setForeground(options.headercolor);
    };

    QString addresslabel;
    if(m_hexdelegate) addresslabel = m_hexdelegate->addressHeader(this);
    if(addresslabel.isEmpty() && !m_options.addresslabel.isEmpty()) addresslabel = m_options.addresslabel;

    QTextCharFormat cf;
    RESET_FORMAT(m_options, cf);
    if(m_hexdelegate) m_hexdelegate->renderHeaderPart(addresslabel, HexArea::Address, cf, this);

    c.insertText(m_fontmetrics.elidedText((" " + addresslabel + " ").leftJustified(this->addressWidth() + 1), Qt::ElideRight, this->hexColumnX()) + " ", cf);
    if(m_hexdelegate) RESET_FORMAT(m_options, cf);

    QString hexlabel;
    if(m_hexdelegate) hexlabel = m_hexdelegate->hexHeader(this);
    if(hexlabel.isEmpty()) hexlabel = m_options.hexlabel;

    if(hexlabel.isEmpty())
    {
        c.insertText(" ", { });

        for(auto i = 0u; i < m_options.linelength; i += m_options.grouplength)
        {
            QString h = QString::number(i, 16).rightJustified(m_options.grouplength * 2, '0').toUpper();

            if(m_hexdelegate)
            {
                RESET_FORMAT(m_options, cf);
                m_hexdelegate->renderHeaderPart(h, HexArea::Hex, cf, this);
            }

            c.insertText(h, cf);
            c.insertText(" ", { });
        }
    }
    else
    {
        if(m_hexdelegate) m_hexdelegate->renderHeaderPart(hexlabel, HexArea::Hex, cf, this);
        c.insertText(" ", cf);
        c.insertText(m_fontmetrics.elidedText(hexlabel.leftJustified(this->hexColumnWidth() / this->cellWidth()), Qt::ElideRight, this->hexColumnWidth()), cf);
        c.insertText(" ", cf);
    }

    if(m_hexdelegate) RESET_FORMAT(m_options, cf);

    QString asciilabel;
    if(m_hexdelegate) asciilabel = m_hexdelegate->asciiHeader(this);
    if(asciilabel.isEmpty()) asciilabel = m_options.asciilabel;

    if(asciilabel.isEmpty())
    {
        c.insertText(" ", { });

        for(unsigned int i = 0; i < m_options.linelength; i++)
        {
            QString a = QString::number(i, 16).toUpper();

            if(m_hexdelegate)
            {
                RESET_FORMAT(m_options, cf);
                m_hexdelegate->renderHeaderPart(a, HexArea::Ascii, cf, this);
            }

            c.insertText(a, cf);
        }

        c.insertText(" ", { });
    }
    else
    {
        if(m_hexdelegate) m_hexdelegate->renderHeaderPart(asciilabel, HexArea::Ascii, cf, this);
        c.insertText(" ", cf);
        c.insertText(m_fontmetrics.elidedText(asciilabel.leftJustified(m_options.linelength),
                                              Qt::ElideRight, this->endColumnX() - this->asciiColumnX() - this->cellWidth()), cf);
        c.insertText(" ", cf);
    }

    QTextBlockFormat bf;
    if(m_options.hasFlag(QHexFlags::StyledHeader)) bf.setBackground(this->palette().color(QPalette::Window));
    if(m_hexdelegate) m_hexdelegate->renderHeader(bf, this);
    c.setBlockFormat(bf);
    c.insertBlock();
}

void QHexView::drawDocument(QTextCursor& c) const
{
    if(!m_hexdocument) return;

    qreal y = !m_options.hasFlag(QHexFlags::NoHeader) ? this->lineHeight() : 0;
    quint64 line = static_cast<quint64>(this->verticalScrollBar()->value());

    QTextCharFormat addrformat;
    addrformat.setForeground(this->palette().color(QPalette::Normal, QPalette::Highlight));

    for(qint64 l = 0; m_hexdocument->isEmpty() || (line < this->lines() && l < this->visibleLines()); l++, line++, y += this->lineHeight())
    {
        quint64 address = line * m_options.linelength + this->baseAddress();
        QString addrstr = QString::number(address, 16).rightJustified(this->addressWidth(), '0').toUpper();

        // Address Part
        QTextCharFormat acf;
        acf.setForeground(m_options.headercolor);

        if(m_options.hasFlag(QHexFlags::StyledAddress))
            acf.setBackground(this->palette().color(QPalette::Window));

        if(m_hexdelegate) m_hexdelegate->renderAddress(address, acf, this);
        c.insertText(" " + addrstr + " ", acf);

        auto linebytes = this->getLine(line);
        c.insertText(" ", { });

        // Hex Part
        for(auto column = 0u; column < m_options.linelength; )
        {
            QTextCharFormat cf;

            for(auto byteidx = 0u; byteidx < m_options.grouplength; byteidx++, column++)
            {
                QString s = linebytes.isEmpty() || column >= static_cast<qint64>(linebytes.size()) ? "  " : QString(QHexUtils::toHex(linebytes.mid(column, 1)).toUpper());
                quint8 b = static_cast<int>(column) < linebytes.size() ? linebytes.at(column) : 0x00;
                cf = this->drawFormat(c, b, s, HexArea::Hex, line, column, static_cast<int>(column) < linebytes.size());
            }

            c.insertText(" ", cf);
        }

        c.insertText(" ", { });

        // Ascii Part
        for(auto column = 0u; column < m_options.linelength; column++)
        {
            auto s = linebytes.isEmpty() ||
                     column >= static_cast<qint64>(linebytes.size()) ? QChar(' ') :
                                                                       (std::isprint(static_cast<quint8>(linebytes.at(column))) ? QChar(linebytes.at(column)) : m_options.unprintablechar);

            quint8 b = static_cast<int>(column) < linebytes.size() ? linebytes.at(column) : 0x00;
            this->drawFormat(c, b, s, HexArea::Ascii, line, column, static_cast<int>(column) < linebytes.size());
        }

        QTextBlockFormat bf;

        if(m_options.linealternatebackground.isValid() && line % 2)
            bf.setBackground(m_options.linealternatebackground);
        else if(m_options.linebackground.isValid() && !(line % 2))
            bf.setBackground(m_options.linebackground);

        c.setBlockFormat(bf);
        c.insertBlock({});
        if(m_hexdocument->isEmpty()) break;
    }
}

unsigned int QHexView::calcAddressWidth() const
{
    if(!m_hexdocument) return 0;

    auto maxaddr = static_cast<quint64>(m_options.baseaddress + m_hexdocument->length());
    if(maxaddr <= std::numeric_limits<quint32>::max()) return 8;
    return QString::number(maxaddr, 16).size();
}

int QHexView::visibleLines(bool absolute) const
{
    int vl = static_cast<int>(std::ceil(this->height() / this->lineHeight()));
    if(!m_options.hasFlag(QHexFlags::NoHeader)) vl--;
    return absolute ? vl : std::min<int>(this->lines(), vl);
}

qint64 QHexView::getLastColumn(qint64 line) const { return this->getLine(line).size() - 1; }
qint64 QHexView::lastLine() const { return std::max<qint64>(0, this->lines() - 1); }

qreal QHexView::hexColumnWidth() const
{
    int l = 0;

    for(auto i = 0u; i < m_options.linelength; i += m_options.grouplength)
        l += (2 * m_options.grouplength) + 1;

    return this->getNCellsWidth(l);
}

unsigned int QHexView::addressWidth() const
{
    if(!m_hexdocument || m_options.addresswidth) return m_options.addresswidth;
    return this->calcAddressWidth();
}

unsigned int QHexView::lineLength() const { return m_options.linelength; }
bool QHexView::canUndo() const { return m_hexdocument && m_hexdocument->canUndo(); }
bool QHexView::canRedo() const { return m_hexdocument && m_hexdocument->canRedo(); }
quint64 QHexView::offset() const { return m_hexcursor->offset(); }
quint64 QHexView::address() const { return m_hexcursor->address(); }
HexPosition QHexView::position() const { return m_hexcursor->position(); }
HexPosition QHexView::selectionStart() const { return m_hexcursor->selectionStart(); }
HexPosition QHexView::selectionEnd() const { return m_hexcursor->selectionEnd(); }
quint64 QHexView::selectionStartOffset() const { return m_hexcursor->selectionStartOffset(); }
quint64 QHexView::selectionEndOffset() const { return m_hexcursor->selectionEndOffset(); }
quint64 QHexView::baseAddress() const { return m_options.baseaddress; }

quint64 QHexView::lines() const
{
    if(!m_hexdocument) return 0;

    auto lines = static_cast<quint64>(std::ceil(m_hexdocument->length() / static_cast<double>(m_options.linelength)));
    return !m_hexdocument->isEmpty() && !lines ? 1 : lines;
}

qint64 QHexView::find(const QByteArray& ba, HexFindDirection fd) const
{
    qint64 startpos = -1;

    if(m_hexcursor->hasSelection()) startpos = m_hexcursor->selectionStartOffset();
    else startpos = m_hexcursor->offset();

    if(fd == HexFindDirection::Backward) startpos--;
    if(startpos < 0) return -1;

    auto offset = fd == HexFindDirection::Forward ? m_hexdocument->indexOf(ba, startpos) :
                                                    m_hexdocument->lastIndexOf(ba, startpos);

    if(offset > -1)
    {
        m_hexcursor->clearSelection();
        m_hexcursor->move(offset);
        m_hexcursor->select(ba.length());
    }

    return offset;
}

qreal QHexView::hexColumnX() const { return this->getNCellsWidth(this->addressWidth() + 2); }
qreal QHexView::asciiColumnX() const { return this->hexColumnX() + this->hexColumnWidth() + this->cellWidth(); }
qreal QHexView::endColumnX() const { return this->asciiColumnX() + this->getNCellsWidth(m_options.linelength + 1) + this->cellWidth(); }
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

HexPosition QHexView::positionFromPoint(QPoint pt) const
{
    HexPosition pos = HexPosition::invalid();
    auto abspt = this->absolutePoint(pt);

    switch(this->areaFromPoint(pt))
    {
        case HexArea::Hex: {
            pos.column = -1;

            for(qint64 i = 0; i < m_hexcolumns.size(); i++) {
                if(m_hexcolumns.at(i).left() > abspt.x()) break;
                pos.column = i;
            }

            break;
        }

        case HexArea::Ascii: pos.column = std::max<qint64>(std::floor((abspt.x() - this->asciiColumnX()) / this->cellWidth()) - 1, 0); break;
        case HexArea::Address: pos.column = 0; break;
        case HexArea::Header: return HexPosition::invalid();
        default: break;
    }

    pos.line = std::min<qint64>(this->verticalScrollBar()->value() + (abspt.y() / this->lineHeight()), this->lines());
    if(!m_options.hasFlag(QHexFlags::NoHeader)) pos.line = std::max<qint64>(0, pos.line - 1);

    auto docline = this->getLine(pos.line);
    pos.column = std::min<qint64>(pos.column, docline.isEmpty() ? 0 : docline.size());

    qhexview_fmtprint("line: %lld, col: %lld", pos.line, pos.column);
    return pos;
}

QPoint QHexView::absolutePoint(QPoint pt) const { return pt + QPoint(this->horizontalScrollBar()->value(), 0); }

HexArea QHexView::areaFromPoint(QPoint pt) const
{
    pt = this->absolutePoint(pt);
    qreal line = this->verticalScrollBar()->value() + pt.y() / this->lineHeight();

    if(!m_options.hasFlag(QHexFlags::NoHeader) && !std::floor(line)) return HexArea::Header;
    if(pt.x() < this->hexColumnX()) return HexArea::Address;
    if(pt.x() < this->asciiColumnX()) return HexArea::Hex;
    if(pt.x() < this->endColumnX()) return HexArea::Ascii;
    return HexArea::Extra;
}

QTextCharFormat QHexView::drawFormat(QTextCursor& c, quint8 b, const QString& s, HexArea area, qint64 line, qint64 column, bool applyformat) const
{
    QTextCharFormat cf, selcf;
    HexPosition pos{line, column};

    if(applyformat)
    {
        auto offset = m_hexcursor->positionToOffset(pos);
        bool hasdelegate = m_hexdelegate && m_hexdelegate->render(offset, b, cf, this);

        if(!hasdelegate)
        {
            auto it = m_options.bytecolors.find(b);

            if(it != m_options.bytecolors.end())
            {
                if(it->background.isValid()) cf.setBackground(it->background);
                if(it->foreground.isValid()) cf.setForeground(it->foreground);
            }
        }

        const auto* metadataline = m_hexmetadata->find(line);

        if(metadataline)
        {
            for(const auto& metadata : *metadataline)
            {
                if(offset < metadata.begin || offset >= metadata.end) continue;

                if(!hasdelegate)
                {
                    if(metadata.foreground.isValid()) cf.setForeground(metadata.foreground);

                    if(metadata.background.isValid())
                    {
                        cf.setBackground(metadata.background);

                        if(!metadata.foreground.isValid())
                            cf.setForeground(this->getReadableColor(metadata.background));
                    }
                }

                if(!metadata.comment.isEmpty())
                {
                    cf.setUnderlineColor(m_options.commentcolor.isValid() ? m_options.commentcolor : this->palette().color(QPalette::WindowText));
                    cf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::SingleUnderline);
                }

                if(offset == metadata.begin) // Remove previous metadata's style, if needed
                {
                    if(metadata.comment.isEmpty()) selcf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::NoUnderline);
                    if(!metadata.foreground.isValid()) selcf.setForeground(Qt::color1);
                    if(!metadata.background.isValid()) selcf.setBackground(Qt::transparent);
                }

                if(offset < metadata.end - 1 && column < this->getLastColumn(line))
                    selcf = cf;
            }
        }

        if(hasdelegate && column < this->getLastColumn(line)) selcf = cf;
    }

    if(this->hexCursor()->isSelected(line, column))
    {
        auto offset = this->hexCursor()->positionToOffset(pos);
        auto selend = this->hexCursor()->selectionEndOffset();

        cf.setBackground(this->palette().color(QPalette::Normal, QPalette::Highlight));
        cf.setForeground(this->palette().color(QPalette::Normal, QPalette::HighlightedText));
        if(offset < selend && column < this->getLastColumn(line)) selcf = cf;
    }

    if(this->hasFocus() && this->hexCursor()->position() == pos)
    {
        auto cursorbg = this->palette().color(QPalette::Normal, QPalette::WindowText);
        auto cursorfg = this->palette().color(QPalette::Normal, QPalette::Base);
        auto discursorbg = this->palette().color(QPalette::Disabled, QPalette::WindowText);
        auto discursorfg = this->palette().color(QPalette::Disabled, QPalette::Base);

        switch(m_hexcursor->mode())
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

    if(column >= m_options.linelength - 1)
    {
        line++;
        column = 0;
    }
    else
        column++;

    qint64 offset = this->hexCursor()->mode() == QHexCursor::Mode::Insert ? 1 : 0;
    if(select) this->hexCursor()->select(std::min<qint64>(line, this->lines()), std::min<qint64>(column, this->getLastColumn(line) + offset));
    else this->hexCursor()->move(std::min<qint64>(line, this->lines()), std::min<qint64>(column, this->getLastColumn(line) + offset));
}

void QHexView::movePrevious(bool select)
{
    auto line = this->hexCursor()->line(), column = this->hexCursor()->column();

    if(column <= 0)
    {
        if(!line) return;
        column = this->getLine(--line).size() - 1;
    }
    else
        column--;

    if(select) this->hexCursor()->select(std::min<qint64>(line, this->lines()), std::min<qint64>(column, this->getLastColumn(line)));
    else this->hexCursor()->move(std::min<qint64>(line, this->lines()), std::min<qint64>(column, this->getLastColumn(line)));
}

bool QHexView::keyPressMove(QKeyEvent* e)
{
    if(e->matches(QKeySequence::MoveToNextChar) || e->matches(QKeySequence::SelectNextChar))
        this->moveNext(e->matches(QKeySequence::SelectNextChar));
    else if(e->matches(QKeySequence::MoveToPreviousChar) || e->matches(QKeySequence::SelectPreviousChar))
        this->movePrevious(e->matches(QKeySequence::SelectPreviousChar));
    else if(e->matches(QKeySequence::MoveToNextLine) || e->matches(QKeySequence::SelectNextLine))
    {
        if(this->hexCursor()->line() == this->lastLine()) return true;
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
        if(this->lastLine() == this->hexCursor()->line()) return true;
        auto pageline = std::min(this->lastLine(), this->hexCursor()->line() + this->visibleLines());
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
        if(this->lastLine() == this->hexCursor()->line()) return true;
        if(e->matches(QKeySequence::MoveToEndOfDocument)) this->hexCursor()->move(this->lastLine(), this->getLastColumn(this->hexCursor()->line()));
        else this->hexCursor()->select(this->lastLine(), this->getLastColumn(this->lastLine()));
    }
    else if(e->matches(QKeySequence::MoveToStartOfLine) || e->matches(QKeySequence::SelectStartOfLine))
    {
        auto offset = this->hexCursor()->positionToOffset({this->hexCursor()->line(), 0});
        if(e->matches(QKeySequence::MoveToStartOfLine)) this->hexCursor()->move(offset);
        else this->hexCursor()->select(offset);
    }
    else if(e->matches(QKeySequence::SelectEndOfLine) || e->matches(QKeySequence::MoveToEndOfLine))
    {
        auto offset = this->hexCursor()->positionToOffset({this->hexCursor()->line(), this->getLastColumn(this->hexCursor()->line())});
        if(e->matches(QKeySequence::SelectEndOfLine)) this->hexCursor()->select(offset);
        else this->hexCursor()->move(offset);
    }
    else
        return false;

    return true;
}

bool QHexView::keyPressTextInput(QKeyEvent* e)
{
    if(m_readonly || e->text().isEmpty() || (e->modifiers() & Qt::ControlModifier)) return false;

    bool atend = m_hexcursor->offset() >= m_hexdocument->length();
    if(atend && m_hexcursor->mode() == QHexCursor::Mode::Overwrite) return false;

    auto key = static_cast<quint8>(e->text().at(0).toLatin1());

    switch(m_currentarea)
    {
        case HexArea::Hex: {
            if(!std::isxdigit(key)) return false;

            bool ok = false;
            auto val = static_cast<quint8>(QString(key).toUInt(&ok, 16));
            if(!ok) return false;
            m_hexcursor->removeSelection();

            quint8 ch = m_hexdocument->isEmpty() || m_hexcursor->offset() >= m_hexdocument->length() ? '\x00' : m_hexdocument->at(m_hexcursor->offset());
            ch = m_writing ? (ch << 4) | val : val;

            if(!m_writing && (m_hexcursor->mode() == QHexCursor::Mode::Insert)) m_hexdocument->insert(m_hexcursor->offset(), val);
            else m_hexdocument->replace(m_hexcursor->offset(), ch);

            m_writing = !m_writing;
            if(!m_writing)
                this->moveNext();

            break;
        }

        case HexArea::Ascii: {
            if(!std::isprint(key)) return false;
            m_hexcursor->removeSelection();
            if(m_hexcursor->mode() == QHexCursor::Mode::Insert) m_hexdocument->insert(m_hexcursor->offset(), key);
            else m_hexdocument->replace(m_hexcursor->offset(), key);
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
        if(e->matches(QKeySequence::SelectAll)) this->selectAll();
        else if(!m_readonly && e->matches(QKeySequence::Undo)) this->undo();
        else if(!m_readonly && e->matches(QKeySequence::Redo)) this->redo();
        else if(!m_readonly && e->matches(QKeySequence::Cut)) this->cut(m_currentarea != HexArea::Ascii);
        else if(e->matches(QKeySequence::Copy)) this->copy(m_currentarea != HexArea::Ascii);
        else if(!m_readonly && e->matches(QKeySequence::Paste)) this->paste(m_currentarea != HexArea::Ascii);
        else return false;

        return true;
    }

    if(m_readonly) return false;

    switch(e->key())
    {
        case Qt::Key_Backspace:
        case Qt::Key_Delete: {
            if(!m_hexcursor->hasSelection()) {
                auto offset = m_hexcursor->offset();
                if(offset <= 0) return true;

                if(e->key() == Qt::Key_Backspace) m_hexdocument->remove(offset - 1, 1);
                else m_hexdocument->remove(offset, 1);
            }
            else
            {
                auto oldpos = m_hexcursor->selectionStart();
                m_hexcursor->removeSelection();
                m_hexcursor->move(oldpos);
            }

            if(e->key() == Qt::Key_Backspace) this->movePrevious();
            m_writing = false;
            break;
        }

        case Qt::Key_Insert:
            m_writing = false;
            m_hexcursor->switchMode();
            break;

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
            if(m_hexdocument && (m_currentarea == HexArea::Hex || m_currentarea == HexArea::Ascii)) {
                auto* helpevent = static_cast<QHelpEvent*>(e);
                auto pos = this->positionFromPoint(helpevent->pos());
                auto comment = m_hexmetadata->getComment(pos.line, pos.column);
                if(!comment.isEmpty()) QToolTip::showText(helpevent->globalPos(), comment);
                return true;
            }

            break;
        }

        default: break;
    }

    return QAbstractScrollArea::event(e);
}

void QHexView::showEvent(QShowEvent* e)
{
    QAbstractScrollArea::showEvent(e);
    this->checkAndUpdate(true);
}

void QHexView::paintEvent(QPaintEvent*)
{
    if(!m_hexdocument) return;

    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setUndoRedoEnabled(false);
    doc.setDefaultFont(this->font());

    QTextCursor c(&doc);
    QPainter painter(this->viewport());
    this->drawHeader(c);
    this->drawDocument(c);

    painter.translate(-this->horizontalScrollBar()->value(), 0);
    doc.drawContents(&painter);
    this->drawSeparators(&painter);
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
    qhexview_fmtprint("%d", static_cast<int>(area));

    switch(area)
    {
        case HexArea::Address: this->hexCursor()->move(pos.line, 0); break;
        case HexArea::Hex: m_currentarea = area; this->hexCursor()->move(pos); break;
        case HexArea::Ascii: m_currentarea = area; this->hexCursor()->move(pos.line, pos.column); break;
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
        case HexArea::Header: this->viewport()->setCursor(Qt::ArrowCursor); return;
        case HexArea::Address: this->viewport()->setCursor(Qt::ArrowCursor); break;
        default: this->viewport()->setCursor(Qt::IBeamCursor); break;
    }

    if(e->buttons() == Qt::LeftButton)
    {
        auto pos = this->positionFromPoint(e->pos());
        if(!pos.isValid()) return;
        if(area == HexArea::Ascii || area == HexArea::Hex) m_currentarea = area;
        this->hexCursor()->select(pos);
    }
}

void QHexView::wheelEvent(QWheelEvent* e)
{
    e->ignore();
    if(!m_hexdocument || !this->verticalScrollBar()->isVisible()) return;

    auto ydelta = e->angleDelta().y();
    if(ydelta > 0) this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - m_options.scrollsteps);
    else if(ydelta < 0) this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + m_options.scrollsteps);
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

QColor QHexView::getReadableColor(QColor c) const
{
    QPalette palette = this->palette();
    return QHexView::isColorLight(c) ? palette.color(QPalette::Normal, QPalette::WindowText) : palette.color(QPalette::Normal, QPalette::HighlightedText);
}

QByteArray QHexView::selectedBytes() const { return m_hexcursor->hasSelection() ? m_hexdocument->read(m_hexcursor->selectionStartOffset(), m_hexcursor->selectionLength()) : QByteArray{ }; }
QByteArray QHexView::getLine(qint64 line) const { return m_hexdocument ? m_hexdocument->read(line * m_options.linelength, m_options.linelength) : QByteArray{ }; }
