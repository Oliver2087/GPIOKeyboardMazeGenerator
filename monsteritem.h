#ifndef MONSTERITEM_H
#define MONSTERITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>

class QGraphicsRectItem;

class MonsterItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    enum MonsterType {
        DamageMonster, // damage
        SlowMonster    // Slow
    };

    explicit MonsterItem(MonsterType type,
                         const QPixmap &sprite,
                         int maxHp,
                         int cellSize,
                         QGraphicsItem *parent = nullptr);

    // Do not call to avoid conflict
    MonsterType monsterType() const { return m_type; }

    void takeDamage(int amount);
    bool isDead() const { return m_hp <= 0; }

    qreal speed() const { return m_speed; }
    void  setSpeed(qreal s) { m_speed = s; }

private:
    void updateHealthBar();

    MonsterType m_type;
    int m_maxHp;
    int m_hp;
    int m_cellSize;
    qreal m_speed;

    QGraphicsRectItem *m_hpBg;
    QGraphicsRectItem *m_hpFg;
};

#endif // MONSTERITEM_H
