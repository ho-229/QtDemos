/**
 * Notify Widget
 * @brief 右下角提示窗
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "notifywidget.h"

#include <QLabel>
#include <QScreen>
#include <QEventLoop>
#include <QSpacerItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QPropertyAnimation>

#include "countdownbutton.h"

NotifyWidget::NotifyWidget(QWidget *parent, const QString &title,
                           const QString &messsage, const QIcon& icon,
                           const QString &style) :
    QWidget(parent),
    m_iconLabel(new QLabel(this)),
    m_titleLabel(new QLabel(this)),
    m_messageLabel(new QLabel(this)),
    m_hLayout(new QHBoxLayout()),
    m_mLayout(new QHBoxLayout()),
    m_vLayout(new QVBoxLayout(this)),
    m_hSpacer(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum)),
    m_icon(icon),
    m_closeButton(new CountdownButton(this)),
    m_animation(new QPropertyAnimation(this, "pos", this))
{
    // Title and close button
    m_hLayout->addWidget(m_titleLabel);
    m_hLayout->addSpacerItem(m_hSpacer);
    m_hLayout->addWidget(m_closeButton);

    // Icon and message
    m_mLayout->addWidget(m_iconLabel);
    m_mLayout->addWidget(m_messageLabel);
    m_mLayout->setStretch(1, 1);
    m_mLayout->setSpacing(9);

    m_vLayout->addLayout(m_hLayout);
    m_vLayout->addLayout(m_mLayout);

    QFont font = m_titleLabel->font();
    font.setPointSize(15);
    font.setBold(true);
    m_titleLabel->setFont(font);
    m_titleLabel->setText(title);
    m_titleLabel->setMaximumWidth(300);

    if(m_icon.isNull())
        m_iconLabel->hide();
    else
    {
        m_iconLabel->setPixmap(m_icon.pixmap(m_iconLabel->size()));
        m_iconLabel->adjustSize();
    }

    m_messageLabel->setText(messsage);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setMaximumSize(QSize(300, 70));

    m_closeButton->setFixedSize(QSize(30, 30));
    m_closeButton->setIcon(QIcon(":/image/resource/image/close.png"));

    QObject::connect(m_closeButton, &CountdownButton::clicked, this,
                     &NotifyWidget::closeAnimation);

    m_animation->setDuration(350);
    m_animation->setEasingCurve(QEasingCurve::OutQuart);

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                         | Qt::WindowSystemMenuHint);
    this->setStyleSheet(style);
    this->setLayout(m_vLayout);
    this->adjustSize();
}

NotifyWidget::~NotifyWidget()
{

}

void NotifyWidget::setCloseCountdown(int ms)
{
    m_closeButton->conutdownCilk(ms);
}

int NotifyWidget::closeCountdown() const
{
    return m_closeButton->countdown();
}

void NotifyWidget::animatMove(int x, int y)
{
    if(m_animation->state() == QPropertyAnimation::Running)
        m_animation->stop();

    m_animation->setEndValue(QPoint(x, y));
    m_animation->setStartValue(this->pos());

    m_animation->start();
}

int NotifyWidget::leftTime() const
{
    return m_animation->duration() - m_animation->currentLoopTime();
}

void NotifyWidget::showEvent(QShowEvent *event)
{
    this->animatMove(this->x() - this->width(), this->y());

    return QWidget::showEvent(event);
}

void NotifyWidget::closeAnimation()
{
    m_isClosing = true;
    this->animatMove(this->x() + this->width(), this->y());

    QObject::connect(m_animation, &QPropertyAnimation::finished, this,
                     [this]{
                         if(QGuiApplication::primaryScreen()->size().width() == this->x())
                             this->close();
                     });
}
void NotifyWidget::closeEvent(QCloseEvent *event)
{
    emit closed();
    return QWidget::closeEvent(event);
}

void NotifyWidget::mouseReleaseEvent(QMouseEvent *event)
{
    emit clicked();
    return QWidget::mouseReleaseEvent(event);
}
