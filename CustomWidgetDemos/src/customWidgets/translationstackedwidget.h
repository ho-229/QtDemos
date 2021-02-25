/**
 * Animation Stacked Widget
 * @brief 平移动画 Stacked Widget
 * @anchor Ho229
 * @date 2020/12/12
 */

#ifndef TRANSLATIONSTACKEDWIDGET_H
#define TRANSLATIONSTACKEDWIDGET_H

#include <QEasingCurve>
#include <QStackedWidget>

class QVariantAnimation;

class TranslationStackedWidget Q_DECL_FINAL : public QStackedWidget
{
    Q_OBJECT

    Q_PROPERTY(int animationDuration READ animationDuration WRITE setAnimationDuration)
    Q_PROPERTY(QEasingCurve animationEasingCurve READ animationEasingCurve WRITE setAnimationEasingCurve)

public:
    /**
     * @brief TranslationStackedWidget
     * @param parent 父对象
     */
    explicit TranslationStackedWidget(QWidget *parent = nullptr);
    ~TranslationStackedWidget() Q_DECL_OVERRIDE;

    /**
     * @brief 平移到 index
     * @param index 目标 index
     * @param exec 是否启用局部事件循环
     */
    void moveToIndex(int index, bool exec = true);

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

    QPixmap m_animationPixmap;

    QWidget *m_currentWidget = nullptr;
    QWidget *m_nextWidget    = nullptr;

    int m_animationX = 0;
    int m_nextIndex;

    static QPixmap mergePixmap(const QPixmap& leftPixmap, const QPixmap& rightPixmap);

    inline void updateFrame();
    inline void updateAnimation();

    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

};

#endif // TRANSLATIONSTACKEDWIDGET_H
