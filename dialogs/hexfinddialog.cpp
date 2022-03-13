#include "hexfinddialog.h"
#include "../model/qhexutils.h"
#include "../qhexview.h"
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpacerItem>
#include <QMessageBox>
#include <QLabel>
#include <array>

const QString HexFindDialog::BUTTONBOX  = "qhexview_buttonbox";
const QString HexFindDialog::CBFINDMODE = "qhexview_cbfindmode";
const QString HexFindDialog::LEFIND     = "qhexview_lefind";
const QString HexFindDialog::HLAYOUT    = "qhexview_hlayout";
const QString HexFindDialog::GBOPTIONS  = "qhexview_gboptions";
const QString HexFindDialog::RBALL      = "qhexview_rball";
const QString HexFindDialog::RBFORWARD  = "qhexview_rbforward";
const QString HexFindDialog::RBBACKWARD = "qhexview_rbbackward";

HexFindDialog::HexFindDialog(QHexView *parent) : QDialog{parent}
{
    m_hexvalidator = new QRegularExpressionValidator(QRegularExpression{"[0-9A-fa-f \\?]+"}, this);

    this->setWindowTitle(tr("Find..."));

    auto* vlayout = new QVBoxLayout(this);
    auto* gridlayout = new QGridLayout();

    auto* cbfindmode = new QComboBox(this);
    cbfindmode->setObjectName(HexFindDialog::CBFINDMODE);
    cbfindmode->addItem("Text", static_cast<int>(QHexFindMode::Text));
    cbfindmode->addItem("Hex", static_cast<int>(QHexFindMode::Hex));
    cbfindmode->addItem("Int", static_cast<int>(QHexFindMode::Int));
    cbfindmode->addItem("Float", static_cast<int>(QHexFindMode::Float));

    auto* lefind = new QLineEdit(this);
    lefind->setObjectName(HexFindDialog::LEFIND);

    gridlayout->addWidget(new QLabel(tr("Mode:"), this), 0, 0);
    gridlayout->addWidget(cbfindmode, 0, 1);
    gridlayout->addWidget(new QLabel(tr("Find:"), this), 1, 0);
    gridlayout->addWidget(lefind, 1, 1);

    vlayout->addLayout(gridlayout);

    auto* gboptions = new QGroupBox(this);
    gboptions->setObjectName(HexFindDialog::GBOPTIONS);
    gboptions->setTitle(tr("Options"));
    gboptions->setLayout(new QVBoxLayout());

    QGroupBox* gbdirection = new QGroupBox(this);
    gbdirection->setTitle(tr("Find direction"));
    auto* gbvlayout = new QVBoxLayout(gbdirection);

    auto* rball = new QRadioButton("All", gbdirection);
    rball->setObjectName(HexFindDialog::RBALL);
    auto* rbforward = new QRadioButton("Forward", gbdirection);
    rbforward->setObjectName(HexFindDialog::RBFORWARD);
    rbforward->setChecked(true);
    auto* rbbackward = new QRadioButton("Backward", gbdirection);
    rbbackward->setObjectName(HexFindDialog::RBBACKWARD);

    gbvlayout->addWidget(rball);
    gbvlayout->addWidget(rbforward);
    gbvlayout->addWidget(rbbackward);
    gbvlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto* hlayout = new QHBoxLayout();
    hlayout->setObjectName(HexFindDialog::HLAYOUT);
    hlayout->addWidget(gboptions, 1);
    hlayout->addWidget(gbdirection);
    vlayout->addLayout(hlayout, 1);

    auto* buttonbox = new QDialogButtonBox(this);
    buttonbox->setOrientation(Qt::Horizontal);
    buttonbox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttonbox->button(QDialogButtonBox::Ok)->setEnabled(false);

    vlayout->addWidget(buttonbox);

    buttonbox->setObjectName(HexFindDialog::BUTTONBOX);
    buttonbox->button(QDialogButtonBox::Ok)->setText(tr("Find"));

    connect(lefind, &QLineEdit::textChanged, this, &HexFindDialog::validateFind);
    connect(cbfindmode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HexFindDialog::updateFindOptions);
    connect(buttonbox, &QDialogButtonBox::accepted, this, &HexFindDialog::find);
    connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    this->updateFindOptions(-1);
}

QHexView* HexFindDialog::hexView() const { return qobject_cast<QHexView*>(this->parentWidget()); }

void HexFindDialog::updateFindOptions(int)
{
    static const std::array<std::pair<QString, unsigned int>, 8> INT_TYPES = {
        std::make_pair("s8",  QHexFindOptions::SignedInt8),
        std::make_pair("s16", QHexFindOptions::SignedInt16),
        std::make_pair("s32", QHexFindOptions::SignedInt32),
        std::make_pair("s64", QHexFindOptions::SignedInt64),
        std::make_pair("u8",  QHexFindOptions::Int8),
        std::make_pair("u16", QHexFindOptions::Int16),
        std::make_pair("u32", QHexFindOptions::Int32),
        std::make_pair("u64", QHexFindOptions::Int64)
    };

    static const std::array<std::pair<QString, unsigned int>, 2> FLOAT_TYPES = {
        std::make_pair("float",  QHexFindOptions::Float),
        std::make_pair("double", QHexFindOptions::Double)
    };

    QGroupBox* gboptions = this->findChild<QGroupBox*>(HexFindDialog::GBOPTIONS);
    qDeleteAll(gboptions->findChildren<QWidget*>());

    QLineEdit* lefind = this->findChild<QLineEdit*>(HexFindDialog::LEFIND);
    lefind->clear();

    bool ok = false;
    QHexFindMode mode = static_cast<QHexFindMode>(this->findChild<QComboBox*>(HexFindDialog::CBFINDMODE)->currentData().toInt(&ok));
    if(!ok) return;

    QVBoxLayout* vlayout = qobject_cast<QVBoxLayout*>(gboptions->layout());

    m_findoptions = QHexFindOptions::None;

    switch(mode)
    {
        case QHexFindMode::Text: {
            lefind->setValidator(nullptr);
            auto* cbcasesensitive = new QCheckBox("Case sensitive", gboptions);

            connect(cbcasesensitive, &QCheckBox::stateChanged, this, [this](int state) {
                if(state == Qt::Checked) m_findoptions |= QHexFindOptions::CaseSensitive;
                else m_findoptions &= ~QHexFindOptions::CaseSensitive;
            });

            vlayout->addWidget(cbcasesensitive);
            gboptions->setVisible(true);
            break;
        }

        case QHexFindMode::Hex: {
            lefind->setValidator(m_hexvalidator);
            gboptions->setVisible(false);
            break;
        }

        case QHexFindMode::Int: {
            lefind->setValidator(nullptr);

            auto* cbbits = new QComboBox();
            for(const auto& it : INT_TYPES) cbbits->addItem(it.first, it.second);

            QHBoxLayout* hl = new QHBoxLayout();
            hl->addWidget(new QLabel("Type"));
            hl->addWidget(cbbits, 1);
            vlayout->addLayout(hl);
            gboptions->setVisible(true);
            break;
        }

        case QHexFindMode::Float: {
            lefind->setValidator(nullptr);

            bool first = true;
            QVBoxLayout* vl = new QVBoxLayout();

            for(const auto& ft : FLOAT_TYPES) {
                auto* rb = new QRadioButton(ft.first);
                rb->setChecked(first);
                vl->addWidget(rb);
                first = false;
            }

            vlayout->addLayout(vl);
            gboptions->setVisible(true);
            break;
        }
    }
}

void HexFindDialog::validateFind()
{
    auto* lefind = this->findChild<QLineEdit*>(HexFindDialog::LEFIND);
    auto* buttonbox = this->findChild<QDialogButtonBox*>(HexFindDialog::BUTTONBOX);

    buttonbox->button(QDialogButtonBox::Ok)->setEnabled(!lefind->text().isEmpty());
}

void HexFindDialog::find()
{
    QString q = this->findChild<QLineEdit*>(HexFindDialog::LEFIND)->text();
    QHexFindMode mode = static_cast<QHexFindMode>(this->findChild<QComboBox*>(HexFindDialog::CBFINDMODE)->currentData().toUInt());

    QHexFindDirection fd;
    if(this->findChild<QRadioButton*>(HexFindDialog::RBBACKWARD)->isChecked()) fd = QHexFindDirection::Backward;
    else if(this->findChild<QRadioButton*>(HexFindDialog::RBFORWARD)->isChecked()) fd = QHexFindDirection::Forward;
    else fd = QHexFindDirection::All;

    auto offset = this->hexView()->hexCursor()->find(q, this->hexView()->offset(), mode, m_findoptions, fd);
    if(offset == -1) QMessageBox::information(this, tr("Not found"), tr("Cannot find '%1'").arg(q));
}
