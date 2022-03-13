#include "qhexutils.h"
#include "qhexoptions.h"
#include "../qhexview.h"
#include <QGlobalStatic>
#include <unordered_map>
#include <array>

namespace QHexUtils {

static const std::array<char, 16> HEXMAP = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

namespace Pattern {

Q_GLOBAL_STATIC_WITH_ARGS(QString, WILDCARD_BYTE, ("??"))

bool check(QString& p, int& len)
{
    static std::unordered_map<QString, std::pair<QString, size_t>> processed; // Cache processed patterns

    auto it = processed.find(p);

    if(it != processed.end())
    {
        p = it->second.first;
        len = it->second.second;
        return true;
    }

    QString op = p; // Store unprocessed pattern
    p = p.simplified().replace(" ", "");
    if(p.isEmpty() || (p.size() % 2)) return false;

    int wccount = 0;

    for(auto i = 0; i < p.size() - 2; i += 2)
    {
        const auto& hexb = p.mid(i, 2);

        if(hexb == *WILDCARD_BYTE)
        {
            wccount++;
            continue;
        }

        if(!std::isxdigit(hexb.front().toLatin1()) || !std::isxdigit(hexb.back().toLatin1()))
            return false;
    }

    if(wccount >= p.size()) return false;
    len = p.size() / 2;
    processed[op] = {p, len}; // Cache processed pattern
    return true;
}

bool match(const QByteArray& data, const QString& pattern)
{
    for(qint64 i = 0, idx = 0; (i <= (pattern.size() - 2)); i += 2, idx++)
    {
        if(idx >= data.size()) return false;

        const QStringRef& hexb = pattern.midRef(i, 2);
        if(hexb == *WILDCARD_BYTE) continue;

        bool ok = false;
        auto b = static_cast<char>(hexb.toUInt(&ok, 16));
        if(!ok || (b != data.at(idx))) return false;
    }

    return true;
}

int find(const QByteArray& data, int datasize, QString pattern, size_t* patternlen)
{
    if(data.isEmpty() || !datasize) return -1;

    int len = 0;
    if(!Pattern::check(pattern, len) || (len > datasize)) return -1;
    if(patternlen) *patternlen = len;

    for(size_t i = 0; datasize >= len; i++, datasize--)
    {
        if(!datasize) break;
        if(Pattern::match(i ? data.mid(i) : data, pattern)) return i;
    }

    return -1;
}

}

QByteArray toHex(const QByteArray& ba, char sep)
{
    QByteArray hex(sep ? (ba.size() * 3 - 1) : (ba.size() * 2), Qt::Uninitialized);

    for(auto i = 0, o = 0; i < ba.size(); i++)
    {
        if(sep && i) hex[o++] = static_cast<uchar>(sep);
        hex[o++] = HEXMAP.at((ba.at(i) & 0xf0) >> 4);
        hex[o++] = HEXMAP.at(ba.at(i) & 0x0f);
    }

    return hex;
}

QByteArray toHex(const QByteArray& ba) { return QHexUtils::toHex(ba, '\0'); }
qint64 positionToOffset(const QHexOptions* options, QHexPosition pos) { return options->linelength * pos.line + pos.column; }
QHexPosition offsetToPosition(const QHexOptions* options, qint64 offset) { return { offset / options->linelength, offset % options->linelength }; }

qint64 findDefault(const QByteArray& value, qint64 startoffset, const QHexView* hexview, unsigned int options, QHexFindDirection fd)
{
    QHexDocument* hexdocument = hexview->hexDocument();
    if(value.size() > hexdocument->length()) return -1;

    qint64 offset = -1;

    for(auto i = fd == QHexFindDirection::Backward ? hexdocument->length() - 1 : startoffset;
        offset == -1 && (fd == QHexFindDirection::Backward ? (i >= startoffset) : (i < hexdocument->length()));
        fd == QHexFindDirection::Backward ? i-- : i++)
    {
        for(auto j = 0; j < value.size(); j++)
        {
            qint64 curroffset = i + j;
            if(curroffset >= hexdocument->length()) return -1;

            uchar ch1 = hexdocument->at(curroffset);
            uchar ch2 = value.at(j);

            if(!(options & QHexFindOptions::CaseSensitive))
            {
                ch1 = std::tolower(ch1);
                ch2 = std::tolower(ch2);
            }

            if(ch1 != ch2) break;
            if(j == value.size() - 1) offset = i;
        }
    }

    return offset;
}

qint64 findPattern(QString pattern, qint64 startoffset, const QHexView* hexview, unsigned int options)
{
    Q_UNUSED(options);

    QHexDocument* hexdocument = hexview->hexDocument();
    int patternlen = 0, datasize = hexdocument->length();
    if(!Pattern::check(pattern, patternlen) || (patternlen > hexdocument->length())) return -1;

    for(qint64 i = 0, offset = startoffset; offset < hexdocument->length() && datasize >= patternlen; i++, offset++, datasize--)
    {
        if(!datasize) break;
        if(Pattern::match(hexdocument->read(offset, patternlen), pattern)) return i;
    }

    return -1;
}

std::pair<qint64, qint64> find(const QHexView* hexview, QVariant value, QHexFindMode mode, unsigned int options, QHexFindDirection fd)
{
    qint64 offset = -1, size = 0, startoffset = 0;

    QHexCursor* hexcursor = hexview->hexCursor();
    if(fd != QHexFindDirection::All) startoffset = hexcursor->hasSelection() ? hexcursor->selectionStartOffset() : hexcursor->offset();

    switch(mode)
    {
        case QHexFindMode::Text: {
            QByteArray v;

            if(value.type() == QVariant::String) v = value.toString().toUtf8();
            else if(value.type() == QVariant::ByteArray) v = value.toByteArray();
            else return {-1, 0};

            offset = QHexUtils::findDefault(v, startoffset, hexview, options, fd);
            size = v.size();
            break;
        }

        case QHexFindMode::Hex: {
            if(value.type() == QVariant::String) {
                auto v = value.toString();
                offset = QHexUtils::findPattern(v, startoffset, hexview, options);
                size = v.size() / 2;
            }
            else if(value.type() == QVariant::ByteArray) {
                auto v = value.toByteArray();
                offset = QHexUtils::findDefault(v, startoffset, hexview, options, fd);
                size = v.size();
            }
            else return {-1, 0};
            break;
        }

        default: return {-1, 0};
    }

    return {offset, offset > -1 ? size : 0};
}

}
