/**
 * @brief keyboard Controllor
 * @author Ho 229
 */

#ifndef KEYBOARDCONTROLLOR_H
#define KEYBOARDCONTROLLOR_H

#include <QObject>

class KeyboardControllor : public QObject
{
    Q_OBJECT
public:
    explicit KeyboardControllor(QObject *parent = nullptr);

signals:
    void play();
    void goahead();
    void back();
    void escape();

private:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

};

#endif // KEYBOARDCONTROLLOR_H
