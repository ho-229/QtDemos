/**
 * Progress Dial
 * @brief 进度圆盘按钮
 * @anchor Ho229
 * @date 2021/2/24
 */

#ifndef PROGRESSDIAL_H
#define PROGRESSDIAL_H

#include <QPen>
#include <QDial>

#define TransAngle(x) (x * 16)

class ProgressDial : public QDial
{
    Q_OBJECT
public:
    explicit ProgressDial(QWidget *parent = nullptr);
    virtual ~ProgressDial() Q_DECL_OVERRIDE;

    /**
     * @brief 设置内圈颜色
     * @param color
     */
    void setProgressColor(const QColor color){ m_pen.setColor(color); }

    /**
     * @return 内圈颜色
     */
    QColor progressColor() const { return m_pen.color(); }

    /**
     * @brief 设置内圈宽度
     * @param width
     */
    void setProgressWidth(qreal width){ m_pen.setWidthF(width); }

    /**
     * @return 内圈宽度
     */
    qreal progressWidth() const { return m_pen.widthF(); }

protected:
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    QPen m_pen;

    inline void drawProgress(int start, int angle);

};

#endif // PROGRESSDIAL_H
