#ifndef QHEXCURSOR_H
#define QHEXCURSOR_H

#include <QObject>
#include "gapbuffer.h"

class QHexCursor : public QObject
{
    Q_OBJECT

    public:
        enum SelectedPart  { AddressPart, HexPart, AsciiPart };
        enum InsertionMode { OverwriteMode, InsertMode };

    public:
        explicit QHexCursor(QObject *parent = 0);
        ~QHexCursor();
        QPoint position() const;
        sinteger_t cursorX() const;
        sinteger_t cursorY() const;
        integer_t offset() const;
        integer_t nibbleIndex() const;
        integer_t selectionStart() const;
        integer_t selectionEnd() const;
        integer_t selectionLength() const;
        SelectedPart selectedPart() const;
        InsertionMode insertionMode() const;
        bool isAddressPartSelected() const;
        bool isHexPartSelected() const;
        bool isAsciiPartSelected() const;
        bool isInsertMode() const;
        bool isOverwriteMode() const;
        bool hasSelection() const;
        bool blinking() const;
        bool isSelected(integer_t offset) const;
        void selectStart();
        void selectEnd();
        void selectAll();
        void setPosition(sinteger_t x, sinteger_t y);
        void setOffset(integer_t offset);
        void setOffset(integer_t offset, integer_t nibbleindex);
        void setSelectionEnd(integer_t offset);
        void setSelection(integer_t startoffset, integer_t endoffset);
        void setSelectionRange(integer_t startoffset, integer_t length);
        void setSelectedPart(SelectedPart sp);
        void clearSelection();
        bool removeSelection();
        void moveOffset(sinteger_t c, bool bynibble = false);
        void moveSelection(sinteger_t c);
        void blink(bool b);
        void switchMode();

    protected:
        virtual void timerEvent(QTimerEvent *event);

    signals:
        void blinkChanged();
        void positionChanged();
        void offsetChanged();
        void selectionChanged();
        void selectedPartChanged();
        void insertionModeChanged();

    private:
        static const int CURSOR_BLINK_INTERVAL; // 5ms
        SelectedPart _selectedpart;
        InsertionMode _insertionmode;
        sinteger_t _cursorx, _cursory;
        integer_t _selectionstart, _offset, _nibbleindex;
        int _timerid;
        bool _blink;
};

#endif // QHEXCURSOR_H
