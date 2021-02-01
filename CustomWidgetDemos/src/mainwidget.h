/**
 * @brief Demo
 * @anchor Ho229
 * @date 2020/12/12
 */

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

#include "customWidgets/toast.h"

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
    ~MainWidget();

private slots:
    void on_toastBtn_clicked();

    void on_buttonClicked(int id);

private:
    Ui::MainWidget *ui;

    Toast *m_toast = nullptr;

    QButtonGroup *m_buttonGroup_1 = nullptr;
    QButtonGroup *m_buttonGroup_2 = nullptr;
    QButtonGroup *m_langGroup     = nullptr;

    QTranslator *m_trans = nullptr;

    inline void initUI();
};
#endif // MAINWIDGET_H
