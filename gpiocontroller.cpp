#include "gpiocontroller.h"
#include "playeritem.h"

#include <QFile>
#include <QKeyEvent>
#include <QDebug>

GpioController::GpioController(int step, QObject *parent)
    : QObject(parent),
    m_player(nullptr),
    m_moveLeft(false),
    m_moveRight(false),
    m_moveUp(false),
    m_moveDown(false),
    m_step(step),
    m_gpioLeft(-1),
    m_gpioRight(-1),
    m_gpioUp(-1),
    m_gpioDown(-1)
{
    // poll GPIO at 50 Hz
    m_pollTimer.setInterval(20);
    connect(&m_pollTimer, &QTimer::timeout,
            this, &GpioController::pollInputs);
    m_pollTimer.start();
}

void GpioController::setPlayer(PlayerItem *player)
{
    m_player = player;
    reset();
}

void GpioController::setStep(int step)
{
    m_step = step;
}

void GpioController::reset()
{
    m_moveLeft  = false;
    m_moveRight = false;
    m_moveUp    = false;
    m_moveDown  = false;

    if (m_player)
        m_player->setAction(PlayerItem::Idle);
}

void GpioController::setGpios(int left, int right, int up, int down)
{
    m_gpioLeft  = left;
    m_gpioRight = right;
    m_gpioUp    = up;
    m_gpioDown  = down;
}

// We keep these for interface compatibility with PlayerController,
// but we don't actually use keyboard input here.
bool GpioController::handleKeyPress(QKeyEvent *event)
{
    Q_UNUSED(event);
    return false;
}

bool GpioController::handleKeyRelease(QKeyEvent *event)
{
    Q_UNUSED(event);
    return false;
}

bool GpioController::readGpio(int gpio) const
{
    if (gpio < 0)
        return false;

    QString path = QStringLiteral("/sys/class/gpio/gpio%1/value").arg(gpio);
    QFile f(path);

    if (!f.open(QIODevice::ReadOnly)) {
        // qDebug() << "Failed to read" << path;
        return false;
    }

    char c = 0;
    if (f.read(&c, 1) != 1)
        return false;

    return (c == '1');  // active-high
}

void GpioController::pollInputs()
{
    // Update movement booleans from GPIO pins
    bool left  = readGpio(m_gpioLeft);
    bool right = readGpio(m_gpioRight);
    bool up    = readGpio(m_gpioUp);
    bool down  = readGpio(m_gpioDown);

    m_moveLeft  = left;
    m_moveRight = right;
    m_moveUp    = up;
    m_moveDown  = down;

    if (!m_player)
        return;

    // Set direction & animation based on active movement
    if (isMoving()) {
        // Choose a primary direction; prioritize horizontal, then vertical
        if (m_moveLeft) {
            m_player->setDirection(PlayerItem::Left);
        } else if (m_moveRight) {
            m_player->setDirection(PlayerItem::Right);
        } else if (m_moveUp) {
            m_player->setDirection(PlayerItem::Back);
        } else if (m_moveDown) {
            m_player->setDirection(PlayerItem::Front);
        }
        m_player->setAction(PlayerItem::Run);
    } else {
        m_player->setAction(PlayerItem::Idle);
    }
}

QPointF GpioController::movementDelta() const
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

bool GpioController::isMoving() const
{
    return m_moveLeft || m_moveRight || m_moveUp || m_moveDown;
}
