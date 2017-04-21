#include "qhextheme.h"
#include <QGuiApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QWidget>

QHexTheme::QHexTheme(QObject *parent): QObject(parent)
{
    QWidget* container = static_cast<QWidget*>(parent);

    container->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    container->setBackgroundRole(QPalette::Base);

    this->_selectedcursor = QColor(Qt::lightGray);
    this->_addressbackground = QColor(0xF0, 0xF0, 0xFE);
    this->_addressforeground = QColor(Qt::darkBlue);
    this->_alternateaddressforeground = QColor(Qt::red);
    this->_alternatelinecolor = QColor(0xF0, 0xF0, 0xFE);
    this->_linecolor = QColor(0xFF, 0xFF, 0xA0);
}

QColor QHexTheme::baseColor() const
{
    return this->_basecolor;
}

QColor QHexTheme::selectedCursor() const
{
    return this->_selectedcursor;
}

QColor QHexTheme::addressBackground() const
{
    return this->_addressbackground;
}

QColor QHexTheme::addressForeground() const
{
    return this->_addressforeground;
}

QColor QHexTheme::alternateAddressForeground() const
{
    return this->_alternateaddressforeground;
}

QColor QHexTheme::alternateLineColor() const
{
    return this->_alternatelinecolor;
}

QColor QHexTheme::lineColor() const
{
    return this->_linecolor;
}

void QHexTheme::setBaseColor(const QColor &color)
{
    if(this->_basecolor == color)
        return;

    this->_basecolor = color;
    emit themeChanged();
}

void QHexTheme::setSelectedCursor(const QColor &color)
{
    if(this->_selectedcursor == color)
        return;

    this->_selectedcursor = color;
    emit themeChanged();
}

void QHexTheme::setAddressBackground(const QColor &color)
{
    if(this->_addressbackground == color)
        return;

    this->_addressbackground = color;
    emit themeChanged();
}

void QHexTheme::setAddressForeground(const QColor &color)
{
    if(this->_addressforeground == color)
        return;

    this->_addressforeground = color;
    emit themeChanged();
}

void QHexTheme::setAlternateAddressForeground(const QColor &color)
{
    if(this->_alternateaddressforeground == color)
        return;

    this->_alternateaddressforeground = color;
    emit themeChanged();
}

void QHexTheme::setAlternateLineColor(const QColor &color)
{
    if(this->_alternatelinecolor == color)
        return;

    this->_alternatelinecolor = color;
    emit themeChanged();
}

void QHexTheme::setLineColor(const QColor &color)
{
    if(this->_linecolor == color)
        return;

    this->_linecolor = color;
    emit themeChanged();
}
