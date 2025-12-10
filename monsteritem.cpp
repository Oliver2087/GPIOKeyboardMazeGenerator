#include "monsteritem.h"

#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>

MonsterItem::MonsterItem(MonsterType type,
                         const QPixmap &sprite,
                         int maxHp,
                         int cellSize,
                         QGraphicsItem *parent)
    : QObject(),
    QGraphicsPixmapItem(parent),
    m_type(type),
    m_maxHp(maxHp),
    m_hp(maxHp),
    m_cellSize(cellSize),
    m_speed(4.0),
    m_hpBg(nullptr),
    m_hpFg(nullptr)
{
    setPixmap(sprite);
    setZValue(0.15); // Above floor/walls, slightly below player

    if (m_type == DamageMonster)
        m_speed = 3.0;
    else
        m_speed = 6.0;   // Slow-monster is actually faster

    // ---- Create health bar ----
    qreal barWidth  = cellSize;
    qreal barHeight = 4.0;

    m_hpBg = new QGraphicsRectItem(0, 0, barWidth, barHeight, this);
    m_hpBg->setBrush(QBrush(Qt::red));
    m_hpBg->setPen(Qt::NoPen);

    m_hpFg = new QGraphicsRectItem(0, 0, barWidth, barHeight, this);
    m_hpFg->setBrush(QBrush(Qt::green));
    m_hpFg->setPen(Qt::NoPen);

    // Place health bar above the monster’s head
    QRectF b = boundingRect();
    Q_UNUSED(b);
    qreal y = -barHeight - 2.0;
    qreal x = -barWidth / 2.0;
    m_hpBg->setPos(x, y);
    m_hpFg->setPos(x, y);

    updateHealthBar();
}

void MonsterItem::takeDamage(int amount)
{
    if (amount <= 0 || m_hp <= 0)
        return;

    m_hp -= amount;
    if (m_hp < 0)
        m_hp = 0;

    updateHealthBar();
}

void MonsterItem::updateHealthBar()
{
    if (!m_hpFg)
        return;

    qreal ratio = (m_maxHp > 0) ? (qreal)m_hp / (qreal)m_maxHp : 0.0;
    if (ratio < 0.0)
        ratio = 0.0;

    qreal fullWidth = m_cellSize;
    qreal barHeight = m_hpFg->rect().height();

    m_hpFg->setRect(0, 0, fullWidth * ratio, barHeight);
}
