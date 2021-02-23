/**
 * Progress Button
 * @brief 进度条按钮
 * @anchor Ho229
 * @date 2021/2/22
 */

#ifndef PROGRESSBUTTON_H
#define PROGRESSBUTTON_H

#include <QPen>
#include <QPushButton>

#define TransAngle(x) (x * 16)

#define DEFULT_BUTTON_STYLE "\
QPushButton\
{\
    background:transparent;\
    border-style:none;\
}\
QPushButton:hover\
{\
    background:transparent;\
    border-radius:7px;\
    border:1px solid rgb(119,119,119);\
}\
QPushButton:pressed\
{\
    background:rgb(220, 220, 220);\
}"

class ProgressButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(int maximun READ maximun WRITE setMaximun)
    Q_PROPERTY(int minimun READ minimun WRITE setMinimun)

    Q_PROPERTY(int progressWidth READ progressWidth WRITE setProgressWidth)
    Q_PROPERTY(QColor progressColor READ progressColor WRITE setProgressColor)

public:
    explicit ProgressButton(QWidget *parent = nullptr,
                            const QString &style = DEFULT_BUTTON_STYLE);
    ~ProgressButton() Q_DECL_OVERRIDE;

    void setMaximun(const int value){ m_maximun = qMax(value, m_minimun); }
    void setMinimun(const int value){ m_minimun = qMin(value, m_maximun); }
    void setValue(int value);

    void setProgressWidth(const int width){ m_pen.setWidth(width); }
    void setProgressColor(const QColor& color){ m_pen.setColor(color); }

    int maximun() const { return m_maximun; }
    int minimun() const { return m_minimun; }
    int value() const { return m_value; }

    int progressWidth() const { return m_pen.width(); }
    QColor progressColor() const { return m_pen.color(); }

signals:
    void valueChanged(int value);

private:
    int m_maximun = 100;
    int m_minimun = 0;
    int m_value = 0;

    QPen m_pen;

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
};

#endif // PROGRESSBUTTON_H
