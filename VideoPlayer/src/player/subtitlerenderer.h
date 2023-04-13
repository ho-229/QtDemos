/**
 * @brief Subtitle Renderer
 * @anchor Ho 229
 * @date 2021/7/14 + 1 == My birthday
 */

#ifndef SUBTITLERENDERER_H
#define SUBTITLERENDERER_H

#include "ffmpegdecoder.h"

#include <QSharedPointer>
#include <QQuickPaintedItem>

class SubtitleRenderer : public QQuickPaintedItem
{
public:
    explicit SubtitleRenderer(QQuickItem *parent = nullptr);
    ~SubtitleRenderer() Q_DECL_OVERRIDE;

    void render(QSharedPointer<SubtitleFrame> &&frame);

    void updateViewRect(const QRect &rect) { m_viewRect = rect; }

protected:
    void paint(QPainter *painter) Q_DECL_OVERRIDE;

private:
    QRect m_viewRect;
    QSharedPointer<SubtitleFrame> m_subtitle;
};

#endif // SUBTITLERENDERER_H
