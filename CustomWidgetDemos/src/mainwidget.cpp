/**
 * @brief Demo
 * @anchor Ho229
 * @date 2020/12/12
 */

#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QButtonGroup>
#include <QTranslator>
#include <QStyle>

#ifdef Q_OS_WIN
# if _MSC_VER >= 1600
#  pragma execution_character_set("utf-8")
# endif
#endif

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget),
      m_buttonGroup_1(new QButtonGroup(this)),
      m_buttonGroup_2(new QButtonGroup(this)),
      m_langGroup(new QButtonGroup(this)),
      m_trans(new QTranslator(this))
{
    ui->setupUi(this);

    m_buttonGroup_1->addButton(ui->rotate_0, 0);
    m_buttonGroup_1->addButton(ui->rotate_1, 1);
    m_buttonGroup_1->addButton(ui->rotate_2, 2);
    connect(m_buttonGroup_1, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            &MainWidget::on_buttonClicked);

    m_buttonGroup_2->addButton(ui->translation_0, 0);
    m_buttonGroup_2->addButton(ui->translation_1, 1);
    m_buttonGroup_2->addButton(ui->translation_3, 2);
    connect(m_buttonGroup_2, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            &MainWidget::on_buttonClicked);

    m_langGroup->addButton(ui->EnBtn, 0);
    m_langGroup->addButton(ui->CnBtn, 1);
    connect(m_langGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            &MainWidget::on_buttonClicked);

    ui->pushButton->setLeftIcon(QApplication::style()->
                                standardIcon(QStyle::SP_MessageBoxInformation));
    ui->pushButton->setRightIcon(QApplication::style()->
                                 standardIcon(QStyle::SP_MessageBoxWarning));
    connect(ui->sideMarginEdit, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int value){
        ui->pushButton->setSideMargin(value);
        ui->pushButton->repaint();
    });
    connect(ui->topBottomMarginEdit, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int value){
        ui->pushButton->setTopBottomMargin(value);
        ui->pushButton->repaint();
    });

    m_trans->load(":/translations/CustomWidgetDemos_zh_CN.qm");
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::on_toastBtn_clicked()
{
    if(m_toast == nullptr)
        m_toast = new Toast(this);

    QString text = ui->toastEdit->text();
    if(text.isEmpty())
        m_toast->toast(tr("Please enter the tip text."));
    else
        m_toast->toast(ui->toastEdit->text());
}

void MainWidget::on_buttonClicked(int id)
{
    QObject *sender = this->sender();

    if(sender == m_buttonGroup_1)       // Rotate Stacked Widget
        ui->rotateStackedWidget->rotate(id);
    else if(sender == m_buttonGroup_2)  // Translation Stacked Widget
        ui->translationStackedWidget->moveToIndex(id);
    else if(sender == m_langGroup)      // Language
    {
        if(id == 0)
            QApplication::removeTranslator(m_trans);
        else
            QApplication::installTranslator(m_trans);
        ui->retranslateUi(this);
    }
}
