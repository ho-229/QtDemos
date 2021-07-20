/**
 * Progress Dial
 * @brief 进度圆盘按钮
 * @anchor Ho229
 * @date 2021/2/24
 */

#include "progressdial.h"

#include <QPainter>

ProgressDial::ProgressDial(QWidget *parent) : QDial(parent)
{
    // init Pen
    m_pen.setWidth(5);
    m_pen.setColor(QColor(55, 120, 249));
}

ProgressDial::~ProgressDial()
{

}

void ProgressDial::paintEvent(QPaintEvent *event)
{
    QDial::paintEvent(event);

    if(this->wrapping())
        this->drawProgress(-90, 360);
    else
        this->drawProgress(-120, 290);
}

void ProgressDial::drawProgress(int start, int angle)
{
    int drawAngle = static_cast<int>(
        static_cast<qreal>(this->value()) / (this->maximum() - this->minimum()) * angle);
    if(drawAngle <= 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(m_pen);

    QRect rect;
    int min = qMin(this->width(), this->height()) - m_pen.width() * 2;
    min -= min / 6;
    rect.setSize(QSize(min, min));
    rect.moveCenter(this->rect().center());

    painter.drawArc(rect, TransAngle(start), TransAngle(start - drawAngle));
}
