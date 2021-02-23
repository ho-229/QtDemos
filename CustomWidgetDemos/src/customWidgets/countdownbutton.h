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

    void setCountdown(int ms);
    int countdown() const;

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
