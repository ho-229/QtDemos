/**
 * Animation Stacked Widget
 * @brief 平移动画 Stacked Widget
 * @anchor Ho229
 * @date 2020/12/12
 */

#include "translationstackedwidget.h"

#include <QPainter>
#include <QEventLoop>
#include <QVariantAnimation>

TranslationStackedWidget::TranslationStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_animation(new QVariantAnimation(this))
{
    m_animation->setDuration(400);
    m_animation->setEasingCurve(QEasingCurve::InOutQuart);

    connect(m_animation, &QVariantAnimation::finished, this,
            &TranslationStackedWidget::on_finished);
    connect(m_animation, &QVariantAnimation::valueChanged,
            [this](const QVariant value){
        m_animationX = value.toInt();
        this->repaint();
    });
}

TranslationStackedWidget::~TranslationStackedWidget()
{

}

void TranslationStackedWidget::moveToIndex(int index, bool exec)
{
    if(m_animation->state() == QVariantAnimation::Running || this->currentIndex() == index)
        return;

    m_nextIndex = index;

    m_currentWidget = this->currentWidget();
    m_nextWidget = this->widget(index);

    if(m_nextWidget == nullptr)
        return;

    this->updateFrame();
    this->updateAnimation();

    m_currentWidget->hide();
    m_animation->start();

    if(exec)
    {
        QEventLoop loop;
        connect(m_animation, &QVariantAnimation::finished, &loop, &QEventLoop::quit);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

void TranslationStackedWidget::setAnimationDuration(int duration)
{
    m_animation->setDuration(duration);
}

int TranslationStackedWidget::animationDuration()
{
    return m_animation->duration();
}

void TranslationStackedWidget::setAnimationEasingCurve(const QEasingCurve &easing)
{
    m_animation->setEasingCurve(easing);
}

QEasingCurve TranslationStackedWidget::animationEasingCurve()
{
    return m_animation->easingCurve();
}

void TranslationStackedWidget::on_finished()
{
    m_nextWidget->show();
    m_nextWidget->raise();

    this->setCurrentIndex(m_nextIndex);
    this->repaint();
}

QPixmap TranslationStackedWidget::mergePixmap(const QPixmap &leftPixmap, const QPixmap &rightPixmap)
{
    QPixmap newPixmap(leftPixmap.width() + rightPixmap.width(),
                      qMax(leftPixmap.height(), rightPixmap.height()));

    QPainter painter(&newPixmap);

    painter.drawPixmap(QPoint(0, 0), leftPixmap);
    painter.drawPixmap(QPoint(leftPixmap.width(), 0), rightPixmap);

    return newPixmap;
}

void TranslationStackedWidget::updateFrame()
{
    m_nextWidget->setGeometry(0, 0, this->width(), this->height());
    m_currentWidget->setGeometry(0, 0, this->width(), this->height());

    m_nextWidget->setGeometry(0, 0, this->width(), this->height());

    QPixmap currentPixmap(m_currentWidget->size());
    m_currentWidget->render(&currentPixmap);

    QPixmap nextPixmap(m_nextWidget->size());
    m_nextWidget->render(&nextPixmap);

    if(m_nextIndex < this->currentIndex())
        m_animationPixmap = mergePixmap(nextPixmap, currentPixmap);
    else
        m_animationPixmap = mergePixmap(currentPixmap, nextPixmap);
}


void TranslationStackedWidget::updateAnimation()
{
    if(m_nextIndex < this->currentIndex())
    {
        // Last page
        m_animation->setStartValue(0 - this->width());
        m_animation->setEndValue(0);
    }
    else
    {
        // Next page
        m_animation->setStartValue(0);
        m_animation->setEndValue(0 - this->width());
    }
}

void TranslationStackedWidget::paintEvent(QPaintEvent *event)
{
    if(m_animation->state() == QVariantAnimation::Running)
    {
        QPainter painter(this);
        painter.drawPixmap(QPoint(m_animationX, 0), m_animationPixmap);
    }
    else
        QStackedWidget::paintEvent(event);
}

void TranslationStackedWidget::resizeEvent(QResizeEvent *event)
{
    if(m_animation->state() == QVariantAnimation::Running)
    {
        this->updateFrame();
        this->updateAnimation();
    }

    QStackedWidget::resizeEvent(event);
}
