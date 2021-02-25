/**
 * @brief Demo
 * @anchor Ho229
 * @date 2020/12/12
 */

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

#include "customWidgets/toast.h"
#include "customWidgets/notifymanager.h"

class QTranslator;
class QButtonGroup;
class QAbstractButton;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget() Q_DECL_OVERRIDE;

private slots:
    void on_toastBtn_clicked();

    void on_buttonClicked(int id);

    void on_countdownStartBtn_clicked();

    void on_countdownBtn_clicked();

    void on_notifyBtn_clicked();

private:
    Ui::MainWidget *ui;

    Toast *m_toast = nullptr;

    QButtonGroup *m_buttonGroup_1  = nullptr;
    QButtonGroup *m_buttonGroup_2  = nullptr;
    QButtonGroup *m_langGroup      = nullptr;

    NotifyManager *m_notifyManager = nullptr;

    QTranslator *m_trans = nullptr;

    inline void initUI();
};
#endif // MAINWIDGET_H
