/**
 * Notify Widget
 * @brief 右下角提示窗
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "notifywidget.h"

#include <QLabel>
#include <QSpacerItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>

#include "countdownbutton.h"

NotifyWidget::NotifyWidget(QWidget *parent, const QString &title,
                           const QString &messsage, const QString &style) :
    QWidget(parent),
    m_titleLabel(new QLabel(this)),
    m_messageLabel(new QLabel(this)),
    m_hLayout(new QHBoxLayout()),
    m_vLayout(new QVBoxLayout(this)),
    m_hSpacer(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum)),
    m_closeButton(new CountdownButton(this)),
    m_animation(new QPropertyAnimation(this, "pos"))
{
    m_hLayout->addWidget(m_titleLabel);
    m_hLayout->addSpacerItem(m_hSpacer);
    m_hLayout->addWidget(m_closeButton);

    m_vLayout->addLayout(m_hLayout);
    m_vLayout->addWidget(m_messageLabel);

    QFont font = m_titleLabel->font();
    font.setPointSize(15);
    font.setBold(true);
    m_titleLabel->setFont(font);
    m_titleLabel->setText(title);
    m_titleLabel->setMaximumWidth(300);

    m_messageLabel->setText(messsage);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setMaximumSize(QSize(300, 100));

    m_closeButton->setFixedSize(QSize(30, 30));
    m_closeButton->setIcon(QIcon(":/image/resource/image/close.png"));

    QObject::connect(m_closeButton, &CountdownButton::clicked, this,
                     &NotifyWidget::closeAnimation);

    m_animation->setDuration(350);

    this->setLayout(m_vLayout);
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                         | Qt::WindowSystemMenuHint);
    this->setStyleSheet(style);
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

void NotifyWidget::closeAnimation()
{
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    m_animation->setStartValue(this->pos());
    m_animation->setEndValue(QPoint(this->x() + this->width(), this->y()));
    m_animation->start();

    QObject::connect(m_animation, &QPropertyAnimation::finished,
                     this, &NotifyWidget::close);
}

void NotifyWidget::showEvent(QShowEvent *event)
{
    m_animation->setEasingCurve(QEasingCurve::InCubic);
    m_animation->setStartValue(QPoint(this->x() + this->width(), this->y()));
    m_animation->setEndValue(this->pos());
    m_animation->start();

    return QWidget::showEvent(event);
}

void NotifyWidget::closeEvent(QCloseEvent *event)
{
    emit closed();
    return QWidget::closeEvent(event);
}
