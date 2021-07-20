/**
 * Click Wave Effect
 * @anchor Ho 229
 * @date 2021/7/18
 */

#ifndef CLICKWAVEEFFECT_H
#define CLICKWAVEEFFECT_H

#include <QQuickPaintedItem>

class QVariantAnimation;
class QSequentialAnimationGroup;

class ClickWaveEffect : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int waveDuration READ waveDuration WRITE setWaveDuration NOTIFY waveDurationChanged)
    Q_PROPERTY(int maxRadius READ maxRadius WRITE setMaxRadius NOTIFY maxRadiusChanged)

public:
    explicit ClickWaveEffect(QQuickItem *parent = nullptr);
    virtual ~ClickWaveEffect() Q_DECL_OVERRIDE;

    void setTarget(QQuickItem *target);
    QQuickItem* target() const { return m_target; }

    void setColor(const QColor& color);
    QColor color() const { return m_color; }

    void setWaveDuration(int duration);
    int waveDuration() const;

    void setMaxRadius(int radius);
    int maxRadius() const { return m_maxRadius; }

signals:
    void waveDurationChanged(int duration);
    void targetChanged(QQuickItem *target);
    void maxRadiusChanged(int radius);
    void colorChanged(QColor color);
    void finished();

protected:
    virtual void paint(QPainter *painter) Q_DECL_OVERRIDE;
    virtual bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private slots:
    void on_finished();

private:
    int m_radius = 0;
    int m_maxRadius = -1;
    bool m_isPressed = false;

    QQuickItem *m_target = nullptr;

    QPoint m_pos;
    QColor m_color;
    QColor m_currentColor;

    QSequentialAnimationGroup *m_animation = nullptr;
    QVariantAnimation *m_alphaAnimation    = nullptr;
    QVariantAnimation *m_radiusAnimation   = nullptr;

    static int maxRadius(const QPoint& pos, const QSize &size);
};

#endif // CLICKWAVEEFFECT_H
