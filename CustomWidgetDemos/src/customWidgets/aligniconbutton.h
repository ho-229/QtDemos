/**
 * @brief 左右 icon 对齐 Push Button
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef ALIGNICONBUTTON_H
#define ALIGNICONBUTTON_H

#include <QPushButton>

class AlignIconButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QIcon leftIcon READ leftIcon WRITE setLeftIcon)
    Q_PROPERTY(QIcon rightIcon READ rightIcon WRITE setRightIcon)
    Q_PROPERTY(int sideMargin READ sideMargin WRITE setSideMargin)
    Q_PROPERTY(int topBottomMargin READ topBottomMargin WRITE setTopBottomMargin)

public:
    explicit AlignIconButton(QWidget *parent = nullptr);
    virtual ~AlignIconButton() Q_DECL_OVERRIDE;

    /**
     * @brief 设置左对齐 icon
     */
    void setLeftIcon(QIcon icon)
    {
        m_leftIcon = icon;
        this->repaint();
    }

    /**
     * @return 左对齐 icon
     */
    QIcon leftIcon() const { return m_leftIcon; }

    /**
     * @brief 设置右对齐 icon
     */
    void setRightIcon(QIcon icon)
    {
        m_rightIcon = icon;
        this->repaint();
    }

    /**
     * @return 右对齐 icon
     */
    QIcon rightIcon() const { return m_rightIcon; }

    /**
     * @brief 设置左右边距
     * @warning 效果将在下次 repaint() 生效
     */
    void setSideMargin(int margin){ m_sideMargin = margin; }

    /**
     * @return 左右边距
     */
    int sideMargin() const { return m_sideMargin; }

    /**
     * @brief 设置上下边距
     * @warning 效果将在下次 repaint() 生效
     */
    void setTopBottomMargin(int margin){ m_topBottomMargin = margin; }

    /**
     * @return 上下边距
     */
    int topBottomMargin() const { return m_topBottomMargin; }

protected:
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    QIcon m_leftIcon;
    QIcon m_rightIcon;

    int m_sideMargin      = 5;
    int m_topBottomMargin = 5;

    inline void drawIcon(QPainter *painter);
};

#endif // ALIGNICONBUTTON_H
