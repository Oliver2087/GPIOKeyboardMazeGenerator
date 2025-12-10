#include "playercontroller.h"
#include "playeritem.h"

#include <QKeyEvent>

PlayerController::PlayerController(int step)
    : m_player(nullptr),
    m_moveLeft(false),
    m_moveRight(false),
    m_moveUp(false),
    m_moveDown(false),
    m_step(step)
{
}

void PlayerController::setPlayer(PlayerItem *player)
{
    m_player = player;
    reset();
}

void PlayerController::setStep(int step)
{
    m_step = step;
}

void PlayerController::reset()
{
    m_moveLeft  = false;
    m_moveRight = false;
    m_moveUp    = false;
    m_moveDown  = false;

    if (m_player)
        m_player->setAction(PlayerItem::Idle);
}

bool PlayerController::handleKeyPress(QKeyEvent *event)
{
    if (!m_player)
        return false;

    if (m_player->action() == PlayerItem::Dying) {
        event->ignore();
        return true;    // we consumed it, but no movement
    }

    if (event->isAutoRepeat()) {
        event->ignore();
        return true;
    }

    bool handled = true;

    switch (event->key()) {
    case Qt::Key_Left:
        m_moveLeft  = true;
        m_moveRight = false;
        m_player->setDirection(PlayerItem::Left);
        m_player->setAction(PlayerItem::Run);
        break;

    case Qt::Key_Right:
        m_moveRight = true;
        m_moveLeft  = false;
        m_player->setDirection(PlayerItem::Right);
        m_player->setAction(PlayerItem::Run);
        break;

    case Qt::Key_Up:
        m_moveUp   = true;
        m_moveDown = false;
        m_player->setDirection(PlayerItem::Back);
        m_player->setAction(PlayerItem::Run);
        break;

    case Qt::Key_Down:
        m_moveDown = true;
        m_moveUp   = false;
        m_player->setDirection(PlayerItem::Front);
        m_player->setAction(PlayerItem::Run);
        break;

    case Qt::Key_Space:
        // Attack in current direction; no movement state change
        m_player->setAction(PlayerItem::Attack);
        break;

    default:
        handled = false;
        break;
    }

    return handled;
}

bool PlayerController::handleKeyRelease(QKeyEvent *event)
{
    if (!m_player)
        return false;

    if (m_player->action() == PlayerItem::Dying) {
        event->ignore();
        return true;
    }

    if (event->isAutoRepeat()) {
        event->ignore();
        return true;
    }

    bool handled = true;

    switch (event->key()) {
    case Qt::Key_Left:
        m_moveLeft = false;
        break;
    case Qt::Key_Right:
        m_moveRight = false;
        break;
    case Qt::Key_Up:
        m_moveUp = false;
        break;
    case Qt::Key_Down:
        m_moveDown = false;
        break;
    default:
        handled = false;
        break;
    }

    if (!isMoving() && m_player)
        m_player->setAction(PlayerItem::Idle);

    return handled;
}

QPointF PlayerController::movementDelta() const
{
    QPointF delta(0, 0);

    if (!m_player)
        return delta;

    if (m_moveLeft)  delta.rx() -= m_step;
    if (m_moveRight) delta.rx() += m_step;
    if (m_moveUp)    delta.ry() -= m_step;
    if (m_moveDown)  delta.ry() += m_step;

    return delta;
}

bool PlayerController::isMoving() const
{
    return m_moveLeft || m_moveRight || m_moveUp || m_moveDown;
}
