/**
 * @brief keyboard Controllor
 * @author Ho 229
 */

#include "keyboardcontrollor.h"

#include <QKeyEvent>

KeyboardControllor::KeyboardControllor(QObject *parent) : QObject(parent)
{

}

bool KeyboardControllor::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        switch(keyEvent->key())
        {
        case Qt::Key_Space:
            emit play();
            break;
        case Qt::Key_Left:
            emit back();
            break;
        case Qt::Key_Right:
            emit goahead();
            break;
        case Qt::Key_Escape:
            emit escape();
            break;
        }
    }

    return QObject::eventFilter(obj, event);
}
