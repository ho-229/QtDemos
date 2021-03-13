/**
 * Notify Widget
 * @brief 右下角提示窗
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QSpacerItem;
class CountdownButton;
class QPropertyAnimation;

#define DEFULT_NOTIFY_STYLE "\
NotifyWidget\
{\
    background:rgb(46, 47, 48);\
    border-style:none;\
}\
QLabel\
{\
    color:white;\
}"

class NotifyWidget Q_DECL_FINAL : public QWidget
{
    Q_OBJECT
public:
    explicit NotifyWidget(QWidget *parent = nullptr, const QString& title = "",
                          const QString& message = "",
                          const QString& style = DEFULT_NOTIFY_STYLE);
    ~NotifyWidget() Q_DECL_OVERRIDE;

    void setCloseCountdown(int ms);
    int closeCountdown() const;

    void animatMove(int x, int y);

    bool isClosing() const { return m_isClosing; }

signals:
    void closed();

private slots:
    void closeAnimation();

private:
    QLabel *m_titleLabel   = nullptr;
    QLabel *m_messageLabel = nullptr;

    QHBoxLayout *m_hLayout = nullptr;
    QVBoxLayout *m_vLayout = nullptr;
    QSpacerItem *m_hSpacer = nullptr;

    CountdownButton *m_closeButton  = nullptr;

    QPropertyAnimation *m_animation = nullptr;

    bool m_isClosing = false;

    void showEvent(QShowEvent *event)   Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
};

#endif // NOTIFYWIDGET_H
