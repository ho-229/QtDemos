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
    this->setRenderTarget(QQuickPaintedItem::FramebufferObject);
    this->setMipmap(true);
}

SubtitleRenderer::~SubtitleRenderer()
{

}

void SubtitleRenderer::render(QSharedPointer<SubtitleFrame> &&frame)
{
    if(frame == m_subtitle)
        return;

    m_subtitle = frame;
    this->update();
}
void SubtitleRenderer::paint(QPainter *painter)
{
    if(m_subtitle)
        painter->drawImage(m_viewRect, m_subtitle->image);
}

