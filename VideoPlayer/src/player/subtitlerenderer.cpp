/**
 * @brief Subtitle Renderer
 * @anchor Ho 229
 * @date 2021/7/14 + 1 == My birthday
 */

#include "subtitlerenderer.h"

#include <QPainter>

SubtitleRenderer::SubtitleRenderer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{

}

SubtitleRenderer::~SubtitleRenderer()
{

}

void SubtitleRenderer::render(const SubtitleFrame &frame)
{
    m_subtitle = frame;

    if(m_subtitle.pts > 0)
        this->update();
}
void SubtitleRenderer::paint(QPainter *painter)
{
    if(m_subtitle.pts > 0)
        painter->drawImage(QRectF(this->x(), this->y(), this->width(), this->height()),
                           m_subtitle.image);
}

