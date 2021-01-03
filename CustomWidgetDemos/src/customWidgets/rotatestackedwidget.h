/**
 * Animation Stacked Widget
 * @brief 翻转动画 Stacked Widget
 * @anchor Ho229
 * @date 2020/12/12
 */

#ifndef ROTATESTACKEDWIDGET_H
#define ROTATESTACKEDWIDGET_H

#include <QEasingCurve>
#include <QStackedWidget>

class QVariantAnimation;

class RotateStackedWidget : public QStackedWidget
{
    Q_OBJECT

    Q_PROPERTY(int animationDuration READ animationDuration WRITE setAnimationDuration)
    Q_PROPERTY(QEasingCurve animationEasingCurve READ animationEasingCurve WRITE setAnimationEasingCurve)

public:

    /**
     * @brief RotateStackedWidget
     * @param parent 父对象
     */
    explicit RotateStackedWidget(QWidget *parent = nullptr);
    ~RotateStackedWidget() Q_DECL_OVERRIDE;

    /**
     * @brief 翻转到 index
     * @param index 目标 index
     * @param exec 是否启用局部事件循环
     */
    void rotate(int index, bool exec = true);

    /**
     * @brief 设置动画持续时间
     * @param  duration 持续时间(msecs)
     */
    void setAnimationDuration(int duration);

    /**
     * @return 动画持续时间
     */
    int animationDuration();

    /**
     * @brief 设置动画缓和曲线
     * @param  easing 动画缓和曲线
     */
    void setAnimationEasingCurve(const QEasingCurve& easing);
    /**
     * @return 动画缓和曲线
     */
    QEasingCurve animationEasingCurve();

private slots:
    void on_finished();

private:
    QVariantAnimation *m_animation = nullptr;

    int m_nextIndex;
    qreal m_rotateValue = 0;

    QPixmap m_currentPixmap;
    QPixmap m_nextPixmap;

    QWidget *m_currentWidget = nullptr;
    QWidget *m_nextWidget    = nullptr;

    inline void updateFrame();

    void paintEvent(QPaintEvent *event)   Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
};

#endif // ROTATESTACKEDWIDGET_H
