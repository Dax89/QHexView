#pragma once

#include <QDialog>

class QRegularExpressionValidator;
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
        QRegularExpressionValidator* m_hexvalidator;

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
