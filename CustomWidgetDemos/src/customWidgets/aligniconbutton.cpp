/**
 * @brief 左右 icon 对齐 Push Button
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "aligniconbutton.h"

#include <QPainter>

AlignIconButton::AlignIconButton(QWidget *parent) : QPushButton(parent)
{

}

AlignIconButton::~AlignIconButton()
{

}

void AlignIconButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    this->drawIcon(&painter);
}

void AlignIconButton::drawIcon(QPainter *painter)
{
    QRect rect;
    rect.setSize(QSize(this->width() - m_sideMargin * 2,
                       this->height() - m_topBottomMargin * 2));
    rect.moveCenter(this->rect().center());

    if(!m_leftIcon.isNull())
    {
        m_leftIcon.paint(painter, rect, Qt::AlignLeft,
                         this->isEnabled() ? QIcon::Normal : QIcon::Disabled,
                         this->isChecked() ? QIcon::On : QIcon::Off);
    }

    if(!m_rightIcon.isNull())
    {
        m_rightIcon.paint(painter, rect, Qt::AlignRight,
                         this->isEnabled() ? QIcon::Normal : QIcon::Disabled,
                         this->isChecked() ? QIcon::On : QIcon::Off);
    }
}
