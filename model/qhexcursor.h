#pragma once

#include <QObject>
#include "qhexoptions.h"

class QHexDocument;

class QHexCursor : public QObject
{
    Q_OBJECT

    public:
        enum class Mode { Overwrite, Insert };

        struct Position {
            qint64 line; qint64 column;
            static inline Position invalid() { return {-1, -1}; }
            inline bool isValid() const { return line >= 0 && column >= 0; }
            inline bool operator==(const Position& rhs) const { return (line == rhs.line) && (column == rhs.column); }
            inline bool operator!=(const Position& rhs) const { return (line != rhs.line) || (column != rhs.column); }
        };

    private:
        explicit QHexCursor(QHexDocument *parent = nullptr);

    public:
        QHexDocument* document() const;
        Mode mode() const;
        qint64 line() const;
        qint64 column() const;
        qint64 offset() const;
        qint64 selectionStartOffset() const;
        qint64 selectionEndOffset() const;
        qint64 selectionLength() const;
        Position position() const;
        Position selectionStart() const;
        Position selectionEnd() const;
        QByteArray selectedBytes() const;
        bool hasSelection() const;
        bool isLineSelected(qint64 line) const;
        bool isSelected(qint64 line, qint64 column) const;
        void setMode(Mode m);
        void toggleMode();
        void move(qint64 offset);
        void move(qint64 line, qint64 column);
        void move(Position pos);
        void select(qint64 line, qint64 column);
        void select(Position pos);
        void select(qint64 length);
        void removeSelection();
        void clearSelection();

    public:
        qint64 positionToOffset(Position pos) const;
        Position offsetToPosition(qint64 offset) const;
        static qint64 positionToOffset(const QHexOptions* options, Position pos);
        static Position offsetToPosition(const QHexOptions* options, qint64 offset);

    Q_SIGNALS:
        void positionChanged();
        void modeChanged();

    private:
        Mode m_mode{Mode::Overwrite};
        Position m_position{}, m_selection{};

    friend class QHexDocument;
    friend class QHexView;
};

