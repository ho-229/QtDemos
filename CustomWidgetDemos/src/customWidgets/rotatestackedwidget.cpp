/**
 * Animation Stacked Widget
 * @brief 翻转动画 Stacked Widget
 * @anchor Ho229
 * @date 2020/12/12
 */

#include "rotatestackedwidget.h"

#include <QPainter>
#include <QEventLoop>
#include <QVariantAnimation>

RotateStackedWidget::RotateStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_animation(new QVariantAnimation(this))
{
    m_animation->setStartValue(0);
    m_animation->setEndValue(180);
    m_animation->setDuration(500);
    m_animation->setEasingCurve(QEasingCurve::Linear);

    connect(m_animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant value){
        m_rotateValue = value.toInt();
        this->repaint();
    });
    connect(m_animation, &QVariantAnimation::finished, this,
            &RotateStackedWidget::on_finished);
}

RotateStackedWidget::~RotateStackedWidget()
{

}

void RotateStackedWidget::rotate(int index, bool exec)
{
    if(m_animation->state() == QVariantAnimation::Running || this->currentIndex() == index)
        return;

    m_nextIndex = index;

    m_currentWidget = this->currentWidget();
    m_nextWidget = this->widget(m_nextIndex);

    if(m_nextWidget == nullptr)
        return;

    this->updateFrame();

    m_currentWidget->hide();
    m_animation->start();

    if(exec)
    {
        QEventLoop loop;
        connect(m_animation, &QVariantAnimation::finished, &loop, &QEventLoop::quit);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

void RotateStackedWidget::setAnimationDuration(int duration)
{
    m_animation->setDuration(duration);
}

int RotateStackedWidget::animationDuration()
{
    return m_animation->duration();
}

void RotateStackedWidget::setAnimationEasingCurve(const QEasingCurve& easing)
{
    m_animation->setEasingCurve(easing);
}

QEasingCurve RotateStackedWidget::animationEasingCurve()
{
    return m_animation->easingCurve();
}

void RotateStackedWidget::on_finished()
{
    m_rotateValue = 0;

    m_nextWidget->show();
    m_nextWidget->raise();

    this->setCurrentIndex(m_nextIndex);
    this->repaint();
}

void RotateStackedWidget::updateFrame()
{
    m_nextWidget->setGeometry(0, 0, this->width(), this->height());
    m_currentWidget->setGeometry(0, 0, this->width(), this->height());

    m_currentPixmap = QPixmap(m_currentWidget->size());
    m_currentWidget->render(&m_currentPixmap);

    m_nextPixmap = QPixmap(m_nextWidget->size());
    m_nextWidget->render(&m_nextPixmap);
}

void RotateStackedWidget::paintEvent(QPaintEvent *event)
{
    if(m_animation->state() == QVariantAnimation::Running)
    {
        QPainter painter(this);

        QTransform transform;
        if(m_rotateValue > 90)
        {
            transform.translate(this->width() / 2, 0);
            transform.rotate(m_rotateValue + 180, Qt::YAxis);

            painter.setTransform(transform);
            painter.drawPixmap(-1 * this->width() / 2, 0, m_nextPixmap);
        }
        else
        {
            transform.translate(this->width() / 2, 0);
            transform.rotate(m_rotateValue, Qt::YAxis);

            painter.setTransform(transform);
            painter.drawPixmap(-1 * this->width() / 2, 0, m_currentPixmap);
        }
    }
    else
        QStackedWidget::paintEvent(event);
}

void RotateStackedWidget::resizeEvent(QResizeEvent *event)
{
    if(m_animation->state() == QVariantAnimation::Running)
        this->updateFrame();

    QStackedWidget::resizeEvent(event);
}
