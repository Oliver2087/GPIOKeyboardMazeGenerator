#ifndef GPIOCONTROLLER_H
#define GPIOCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QPointF>

class PlayerItem;   // forward declaration
class QKeyEvent;

class GpioController : public QObject
{
    Q_OBJECT

public:
    explicit GpioController(int step = 4, QObject *parent = nullptr);

    // Same interface as PlayerController
    void setPlayer(PlayerItem *player);
    void setStep(int step);
    void reset();

    // Kept for interface compatibility; always return false (no keyboard)
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);

    QPointF movementDelta() const;
    bool isMoving() const;

    // Extra: configure which GPIOs map to directions
    void setGpios(int left, int right, int up, int down);

private slots:
    void pollInputs();

private:
    bool readGpio(int gpio) const;

    PlayerItem *m_player;

    // same logical movement flags as PlayerController
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;

    int m_step;

    // GPIO pin numbers (sysfs indices)
    int m_gpioLeft;
    int m_gpioRight;
    int m_gpioUp;
    int m_gpioDown;

    QTimer m_pollTimer;
};

#endif // GPIOCONTROLLER_H
