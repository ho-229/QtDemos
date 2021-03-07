/**
 * Notify Manager
 * @brief 右下角提示窗管理器
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef NOTIFYMANAGER_H
#define NOTIFYMANAGER_H

#include <QObject>

#include "notifywidget.h"

typedef QPair<NotifyWidget *, int> NotifyItem;

class NotifyManager Q_DECL_FINAL : public QObject
{
    Q_OBJECT
public:
    explicit NotifyManager(QObject *parent = nullptr);
    ~NotifyManager() Q_DECL_OVERRIDE;

    /**
     * @brief 设置最大弹窗数
     */
    void setMaximum(int value){ m_maximum = qMax(1 ,value); }
    int maximum() const { return m_maximum; }

    /**
     * @return 当前弹窗数
     */
    int showCount() const { return m_showCount; }

public slots:
    /**
     * @brief 弹出提示窗
     * @param parent 父对象
     * @param title 提示窗标题
     * @param message 提示消息
     * @param showTime 展示时间 ( 为 -1 时永久展示 )
     */
    void notify(QWidget *parent, QString title, QString message, int showTime = 5000);

signals:
    void windowClosed();

private slots:
    void onNotifyClosed();

private:
    QList<NotifyItem> m_list;
    int m_maximum = 5;
    int m_showCount = 0;

    const QSize m_desktopSize;

    void updateNotifys();

};

#endif // NOTIFYMANAGER_H
