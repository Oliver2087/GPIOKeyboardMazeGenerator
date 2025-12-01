#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QPointF>

class QKeyEvent;
class PlayerItem;

class PlayerController
{
public:
    explicit PlayerController(int step = 15);

    void setPlayer(PlayerItem *player);
    void setStep(int step);

    // return true if event handled
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);

    QPointF movementDelta() const;
    bool isMoving() const;
    void reset();

private:
    PlayerItem *m_player;
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;
    int  m_step;
};

#endif // PLAYERCONTROLLER_H
