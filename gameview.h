#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QPixmap>
#include <QBrush>
#include <vector>
#include <QVector>
#include <QPoint>
#include <QFutureWatcher>
#include <QSet>
#include <QHash>

#include "mazegenerator.h"
#include "monsteritem.h"
#include "playeritem.h"
#include "playercontroller.h"
#include "gpiocontroller.h"
#include "textures.h"
#include "loadingoverlay.h"

class QKeyEvent;
class QResizeEvent;

#define KEY     1
#define GPIO    2
#define CONTROL GPIO

struct WallStyle {
    QString prefix;   // e.g. "brick_brown" or "stone_brick"
    int count;        // number of numbered variants: 0..count-1
};

struct FloorStyle {
    QString prefix;   // e.g. "brick_brown" or "stone_brick"
    int count;        // number of numbered variants: 0..count-1
};

class GameView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GameView(const QString &characterName = "Assassin",
                      QWidget *parent = nullptr);

    void loadNextLevel();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void stepMovement();  // Player movement timer callback (≈60 FPS)
    void finishLoadNextLevel();

private:
    void buildMaze();
    bool tryMovePlayer(const QPointF &delta); // movement + collision handling

    QGraphicsScene       *m_scene;
    PlayerItem           *m_player;
    QGraphicsPixmapItem  *m_exitTile;

    std::vector<QGraphicsPixmapItem*> m_doors;

    QString m_characterName;

    int m_step;
    int m_cellSize;
    int m_rowsCells;
    int m_colsCells;

    TextureSet m_wallSet;
    TextureSet m_floorSet;

    LoadingOverlay *m_loader;
    bool m_isLoading = false;

    // Player movement timer (~60 FPS)
    QTimer m_moveTimer;

    // Previously used async maze generator watcher (unused now but kept)
    QFutureWatcher<MazeGenerator::MazeData> m_mazeWatcher;

    QSet<MonsterItem*> m_touchingMonsters;
    QHash<MonsterItem*, int> m_lastAttackTick;

    // ===== Newly added monster / player status variables =====
    QVector<QPoint>     m_walkableCells;   // All walkable tiles (used for monster spawning)
    QList<MonsterItem*> m_monsters;        // Monsters currently in the scene

    bool m_monstersSpawned = false;        // Whether monsters were spawned for this level
    int  m_playerMaxHp     = 300;
    int  m_playerHp        = 300;
    bool m_playerSlowed    = false;
    int  m_tickCount       = 0;            // Tick counter (controls attack timing)

    // ---- Player HP bar ----
    QGraphicsRectItem *m_playerHpBg = nullptr;
    QGraphicsRectItem *m_playerHpFg = nullptr;

    // ---- Monster AI timer (independent of player input) ----
    QTimer m_monsterAITimer;

    // Saved maze grid (for monster collision: 1 = wall)
    std::vector<std::vector<int>> m_grid;

    // Internal helper functions
    void spawnMonsters(int count = 3);                     // Spawn monsters
    void updateMonsters();                                 // Monster AI & attacks
    void resolvePlayerAttack();                            // Player attack on SPACE
    void damagePlayer(int amount);                         // Reduce player HP
    void applySlowToPlayer(int durationMs, qreal factor);  // Apply slow effect
    void updatePlayerHpBar();                              // Update player HP bar position + size
    bool monsterCanMoveTo(const QPointF &pos);             // Can monsters move to this tile?

#if CONTROL==GPIO
    GpioController m_controller;
#else
    PlayerController m_controller;
#endif
};

#endif // GAMEVIEW_H
