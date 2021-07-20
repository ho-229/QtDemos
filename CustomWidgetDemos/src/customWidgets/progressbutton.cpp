/**
 * Progress Button
 * @brief 进度条按钮
 * @anchor Ho229
 * @date 2021/2/22
 */

#include "progressbutton.h"

#include <QPainter>

ProgressButton::ProgressButton(QWidget *parent, const QString &style) :
    QPushButton(parent)
{
    m_pen.setCapStyle(Qt::RoundCap);
    m_pen.setWidth(3);
    m_pen.setColor(QColor(69, 198, 214));

    this->setStyleSheet(style);
}

ProgressButton::~ProgressButton()
{

}

void ProgressButton::setValue(int value)
{
    value = qMin(value, m_maximun);

    if(value == m_value)        // No changed
        return;

    m_value = value;
    emit valueChanged(value);
    this->repaint();
}

void ProgressButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    int drawAngle = static_cast<int>(
        static_cast<qreal>(m_value) / (m_maximun - m_minimun) * 360);
    if(drawAngle <= 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(m_pen);

    QRect rect;
    int min = qMin(this->width(), this->height()) - m_pen.width() * 2;
    rect.setSize(QSize(min, min));
    rect.moveCenter(this->rect().center());

    painter.drawArc(rect, TransAngle(90), TransAngle(90 - drawAngle));
}
