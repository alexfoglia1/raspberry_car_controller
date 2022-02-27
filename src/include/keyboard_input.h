#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <QThread>

class KeyboardInput
{
    Q_OBJECT
public:
    KeyboardInput();

protected:
    void run() override;

signals:
    void received_keyboard(int key);
private:

};

#endif //USER_INPUT_H

