/**
 * Toast Widget
 * @brief 自动消失提示框
 * @anchor Ho229
 * @date 2020/12/12
 */

#ifndef TOAST_H
#define TOAST_H

#include <QLabel>

class QTimer;
class QHBoxLayout;
class QPropertyAnimation;
class QSequentialAnimationGroup;

#define DEFULT_TOAST_STYLE "\
QLabel\
{\
    color:#FFFFFF;\
    font:15px;\
    font-weight:500;\
    background-color:rgba(0,0,0,150);\
    padding:3px;\
    border-radius:9;\
}"

class Toast Q_DECL_FINAL : public QWidget
{
    Q_OBJECT
public:

    /**
     * @brief Toast
     * @param parent 父对象
     * @param horizontalMargin 水平方向的边界
     * @param verticalMargin 竖直方向上的边界
     * @param maxmaximumWidth 最大宽度
     * @param wordWrap 启用自动换行
     * @param waitMsece 等待时间
     * @param style 提示框样式表:注意字体大小和宽高效果要配合好
     */
    explicit Toast(QWidget *parent = nullptr, int horizontalMargin = 12, int verticalMargin = 12,
                   int maxmaximumWidth = 1400, bool wordWrap = false, int waitMsecs = 1200,
                   const QString &style = DEFULT_TOAST_STYLE);
    ~Toast() Q_DECL_OVERRIDE;

    /**
     * @brief 设置提示文字
     * @param text 提示文字
     */
    void setText(const QString& text);

    /**
     * @brief 弹出提示
     * @warning 此函数不会重新调整弹出位置
     */
    void toast();

    /**
     * @brief 弹出提示
     * @param text 提示文字
     */
    void toast(const QString& text)
    {
        this->setText(text);
        this->toast();
    }

private:
    QLabel *m_messageLabel = nullptr;
    QHBoxLayout *m_layout  = nullptr;

    QSequentialAnimationGroup *m_animation = nullptr;
    QPropertyAnimation *m_posAnimation     = nullptr;   // 弹出动画
    QPropertyAnimation *m_opacityAnimation = nullptr;   // 消失动画

};

#endif // TOAST_H
