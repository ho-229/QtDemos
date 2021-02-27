/**
 * Countdown Button
 * @brief 倒计时 Button
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#ifndef COUNTDOWNBUTTON_H
#define COUNTDOWNBUTTON_H

#include "progressbutton.h"

class QPropertyAnimation;

class CountdownButton : public ProgressButton
{
    Q_OBJECT

    Q_PROPERTY(int countdown READ countdown WRITE setCountdown)

public:
    explicit CountdownButton(QWidget *parent = nullptr);
    virtual ~CountdownButton() Q_DECL_OVERRIDE;

    /**
     * @brief 设置倒计时时长 (毫秒)
     */
    void setCountdown(int ms);
    int countdown() const;

    /**
     * @brief 启动倒计时
     */
    void conutdownCilk();

    void conutdownCilk(int ms)
    {
        this->setCountdown(ms);
        this->conutdownCilk();
    }

private:
    QPropertyAnimation *m_animation = nullptr;

};

#endif // COUNTDOWNBUTTON_H
