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

class NotifyManager : public QObject
{
    Q_OBJECT
public:
    explicit NotifyManager(QObject *parent = nullptr);
    ~NotifyManager() Q_DECL_OVERRIDE;

    void notify(QWidget *parent, QString title, QString message, int showTime = 5000);

    void setMaximum(int value){ m_maximum = qMax(1 ,value); }
    int maximum() const { return m_maximum; }

    int showCount() const { return m_showCount; }

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
