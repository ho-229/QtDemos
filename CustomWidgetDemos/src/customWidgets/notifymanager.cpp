/**
 * Notify Manager
 * @brief 右下角提示窗管理器
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "notifymanager.h"

#include <QTimer>
#include <QScreen>
#include <QEventLoop>
#include <QGuiApplication>

NotifyManager::NotifyManager(QObject *parent) :
    QObject(parent),
    m_desktopSize(QGuiApplication::primaryScreen()->size())
{

}

NotifyManager::~NotifyManager()
{

}

void NotifyManager::notify(QWidget *parent, QString title, QString message, int showTime)
{
    NotifyWidget *newNotofy = new NotifyWidget(parent, title, message);
    QObject::connect(newNotofy, &NotifyWidget::closed, this,
                     &NotifyManager::onNotifyClosed);
    m_list.push_back({newNotofy, showTime});

    this->updateNotifys();

    QEventLoop loop;
    QTimer::singleShot(800, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

void NotifyManager::onNotifyClosed()
{
    NotifyWidget *closedWidget = static_cast<NotifyWidget *>(this->sender());

    const QList<NotifyItem>& constlist = m_list;
    for(const NotifyItem& item : constlist)
    {
        if(item.first == closedWidget)
        {
            m_list.removeOne(item);
            break;
        }
    }
    this->updateNotifys();
    emit windowClosed();
}

void NotifyManager::updateNotifys()
{
    if(m_list.isEmpty())
        return;

    m_showCount = 0;
    for(int i = 0; i < m_list.size() && i < m_maximum; i++)
    {
        NotifyItem item = m_list.at(i);

        if(item.first->isHidden())
        {
            item.first->move(QPoint(m_desktopSize.width(),
                                    m_desktopSize.height() - (i + 1) * 170));
            if(item.second > 0)
                item.first->setCloseCountdown(item.second);

            item.first->show();
        }
        else
            item.first->animatMove(m_desktopSize.width() - item.first->width(),
                                   m_desktopSize.height() - (i + 1) * 170);
        m_showCount++;
    }
}
