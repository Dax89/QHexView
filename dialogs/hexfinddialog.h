#pragma once

#include <QDialog>

class QRegularExpressionValidator;
class QDoubleValidator;
class QIntValidator;
class QHexView;

class HexFindDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit HexFindDialog(QHexView *parent = nullptr);
        QHexView* hexView() const;

    private Q_SLOTS:
        void updateFindOptions(int);
        void validateFind();
        void find();

    private:
        bool validateIntRange(uint v) const;

    private:
        QRegularExpressionValidator* m_hexvalidator;
        QDoubleValidator* m_dblvalidator;
        QIntValidator* m_intvalidator;
        int m_oldidxbits{-1}, m_oldidxendian{-1};
        unsigned int m_findoptions{0};
        qint64 m_startoffset{-1};

    private:
        static const QString BUTTONBOX;
        static const QString CBFINDMODE;
        static const QString LEFIND;
        static const QString HLAYOUT;
        static const QString GBOPTIONS;
        static const QString RBALL;
        static const QString RBFORWARD;
        static const QString RBBACKWARD;
};
