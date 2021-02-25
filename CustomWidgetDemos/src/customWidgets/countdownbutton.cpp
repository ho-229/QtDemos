/**
 * Countdown Button
 * @brief 倒计时 Button
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "countdownbutton.h"

#include <QPropertyAnimation>

CountdownButton::CountdownButton(QWidget *parent) :
    ProgressButton(parent),
    m_animation(new QPropertyAnimation(this, "value"))
{
    m_animation->setDuration(2000);
    m_animation->setStartValue(this->minimun());
    m_animation->setEndValue(this->maximun());
    QObject::connect(m_animation, &QPropertyAnimation::finished, this,
            [this]{
        this->setValue(0);
        this->click();
    });
}

CountdownButton::~CountdownButton()
{

}

void CountdownButton::setCountdown(int ms)
{
    m_animation->setDuration(ms);
}

int CountdownButton::countdown() const
{
    return m_animation->duration();
}

void CountdownButton::conutdownCilk()
{
    if(m_animation->state() == QPropertyAnimation::Running)
        m_animation->stop();

    m_animation->start();
}
