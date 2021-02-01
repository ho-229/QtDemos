/**
 * Toast Widget
 * @brief 自动消失提示框
 * @anchor Ho229
 * @date 2020/12/12
 */

#include "toast.h"

#include <QScreen>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

Toast::Toast(QWidget *parent, int horizontalMargin, int verticalMargin,
             int maximumWidth, bool wordWrap, int waitMsecs, const QString &style) :
    QWidget(parent),
    m_messageLabel(new QLabel(this)),
    m_layout(new QHBoxLayout(this)),
    m_animation(new QSequentialAnimationGroup(this)),
    m_posAnimation(new QPropertyAnimation(this, "pos")),
    m_opacityAnimation(new QPropertyAnimation(this, "windowOpacity"))
{
    this->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::CustomizeWindowHint);

    m_layout->addWidget(m_messageLabel);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_messageLabel->setStyleSheet(style);
    m_messageLabel->setContentsMargins(horizontalMargin, verticalMargin,
                                       horizontalMargin, verticalMargin);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setMaximumWidth(maximumWidth);
    m_messageLabel->setWordWrap(wordWrap);

    /* 显示动画 */
    m_posAnimation->setDuration(300);
    m_posAnimation->setEasingCurve(QEasingCurve::OutCubic);

    /* 消失动画 */
    m_opacityAnimation->setDuration(250);
    m_opacityAnimation->setStartValue(1);
    m_opacityAnimation->setEndValue(0);

    m_animation->addAnimation(m_posAnimation);
    m_animation->addPause(waitMsecs);
    m_animation->addAnimation(m_opacityAnimation);

    connect(m_animation, &QSequentialAnimationGroup::finished, this, &Toast::close);
}

Toast::~Toast()
{

}

void Toast::setText(const QString &text)
{
    // 自适应大小
    m_messageLabel->setText(text);
    m_messageLabel->adjustSize();
    this->adjustSize();

    // 计算动画起止坐标
    QRect rect = this->rect();
    QRect parentGeometry = this->parentWidget() == nullptr ?
        QGuiApplication::primaryScreen()->geometry() : this->parentWidget()->frameGeometry();

    rect.moveCenter(parentGeometry.center());
    rect.translate(0, parentGeometry.height() / 4);

    m_posAnimation->setEndValue(rect.topLeft());

    rect.translate(0, parentGeometry.height() / 4 - this->height() / 2);

    m_posAnimation->setStartValue(rect.topLeft());
}

void Toast::toast()
{
    if(m_animation->state() == QSequentialAnimationGroup::Running)
        m_animation->stop();

    this->setWindowOpacity(1);          // 透明度复位
    this->show();
    m_animation->start();
}
