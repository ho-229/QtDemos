/**
 * Click Wave Effect
 * @anchor Ho 229
 * @date 2021/7/18
 */

#include "clickwaveeffect.h"

#include <QPainter>
#include <QVariantAnimation>
#include <QSequentialAnimationGroup>

ClickWaveEffect::ClickWaveEffect(QQuickItem *parent)
    : QQuickPaintedItem(parent),
    m_animation(new QSequentialAnimationGroup(this)),
    m_alphaAnimation(new QVariantAnimation(this)),
    m_radiusAnimation(new QVariantAnimation(this))
{
    m_color = {207, 207, 207};          // Default color

    m_alphaAnimation->setStartValue(255);
    m_alphaAnimation->setEndValue(0);
    m_alphaAnimation->setDuration(200);

    m_radiusAnimation->setStartValue(0);
    m_radiusAnimation->setDuration(400);

    m_animation->addAnimation(m_radiusAnimation);
    m_animation->addAnimation(m_alphaAnimation);

    QObject::connect(m_radiusAnimation, &QVariantAnimation::valueChanged, this,
                     [this](const QVariant& value){
                         m_radius = value.toInt();
                         this->update();
                     });
    QObject::connect(m_alphaAnimation, &QVariantAnimation::valueChanged, this,
                     [this](const QVariant& value){
                         m_currentColor.setAlpha(value.toInt());
                         this->update();
                     });
    QObject::connect(m_radiusAnimation, &QVariantAnimation::finished, this,
                     [this]{
                         if(m_isPressed)
                             m_animation->pause();
                     });
    QObject::connect(m_animation, &QSequentialAnimationGroup::finished, this,
                     &ClickWaveEffect::on_finished);
}

ClickWaveEffect::~ClickWaveEffect()
{

}

void ClickWaveEffect::setTarget(QQuickItem *target)
{
    if(!target)
        return;

    target->installEventFilter(this);
    m_target = target;

    emit targetChanged(target);
}

void ClickWaveEffect::setColor(const QColor &color)
{
    m_alphaAnimation->setStartValue(color.alpha());
    m_color = color;
    emit colorChanged(color);
}

void ClickWaveEffect::setWaveDuration(int duration)
{
    m_radiusAnimation->setDuration(duration);
    emit waveDurationChanged(duration);
}

int ClickWaveEffect::waveDuration() const
{
    return m_radiusAnimation->duration();
}

void ClickWaveEffect::setMaxRadius(int radius)
{
    m_maxRadius = radius;
    emit maxRadiusChanged(radius);
}

void ClickWaveEffect::paint(QPainter *painter)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_currentColor);
    painter->drawEllipse(m_pos, m_radius, m_radius);
}

bool ClickWaveEffect::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == m_target)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            m_pos = mouseEvent->pos();
            m_isPressed = true;

            if(m_animation->state() == QSequentialAnimationGroup::Running)
            {
                m_animation->stop();
                m_radius = 0;
            }

            m_currentColor = m_color;

            m_radiusAnimation->setEndValue(
                        m_maxRadius < 0 ? maxRadius(m_pos, this->size().toSize()) : m_maxRadius);

            m_animation->start();
        }
        else if(event->type() == QEvent::MouseButtonRelease)
        {
            m_isPressed = false;

            if(m_animation->state() == QSequentialAnimationGroup::Paused)
                m_animation->resume();
        }
    }

    return QQuickPaintedItem::eventFilter(obj, event);
}

void ClickWaveEffect::on_finished()
{
    m_radius = 0;
    emit finished();
}

int ClickWaveEffect::maxRadius(const QPoint &pos, const QSize &size)
{
    int radius = 0;

    auto dist = [](const QPoint& x, const QPoint& y) -> int {
        const QPoint diff = x - y;
        return diff.manhattanLength();
    };

    if(pos.x() < size.width() / 2)          // Left
    {
        if(pos.y() < size.height() / 2)     // Top
            radius = dist(pos, {size.width(), size.height()});  // RightBottom
        else                                // Bottom
            radius = dist(pos, {size.width(), 0});              // RightTop
    }
    else                                    // Right
    {
        if(pos.y() < size.height() / 2)     // Top
            radius = dist(pos, {0, size.height()});             // LeftBottom
        else                                // Bottom
            radius = dist(pos, {0, 0});                         // RightBottom
    }

    return radius;
}
