#ifndef QHEXTHEME_H
#define QHEXTHEME_H

#include <QObject>
#include <QColor>

class QHexTheme: public QObject
{
    Q_OBJECT

    public:
        QHexTheme(QObject* parent = 0);
        QColor baseColor() const;
        QColor selectedCursor() const;
        QColor addressBackground() const;
        QColor addressForeground() const;
        QColor alternateAddressForeground() const;
        QColor alternateLineColor() const;
        QColor lineColor() const;

    public:
        void setBaseColor(const QColor& color);
        void setSelectedCursor(const QColor& color);
        void setAddressBackground(const QColor& color);
        void setAddressForeground(const QColor& color);
        void setAlternateAddressForeground(const QColor& color);
        void setAlternateLineColor(const QColor& color);
        void setLineColor(const QColor& color);

    signals:
        void themeChanged();

    private:
        QColor _basecolor;
        QColor _selectedcursor;
        QColor _addressbackground;
        QColor _addressforeground;
        QColor _alternateaddressforeground;
        QColor _alternatelinecolor;
        QColor _linecolor;
};

#endif // QHEXTHEME_H
