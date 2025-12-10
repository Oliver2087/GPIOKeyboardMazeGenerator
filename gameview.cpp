#include "gameview.h"
#include "mazegenerator.h"
#include "playercontroller.h"

#include <QKeyEvent>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QDir>
#include <QApplication>
#include <QLineF>
#include <QDebug>

GameView::GameView(const QString &characterName, QWidget *parent)
    : QGraphicsView(parent),
    m_scene(new QGraphicsScene(this)),
    m_player(nullptr),
    m_exitTile(nullptr),
    m_characterName(characterName),
    m_step(6),
    m_cellSize(32),
    m_rowsCells(10),
    m_colsCells(15),
    m_loader(nullptr),
#if CONTROL == GPIO
    m_controller(15, this)        // GPIO / OTHER mode
#else
    m_controller(15)              // KEYBOARD mode
#endif
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setScene(m_scene);

    setFixedSize(480, 272);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    m_loader = new LoadingOverlay(this);
    QString base = QCoreApplication::applicationDirPath();
    m_loader->setGif(base + "/texture/loading_screen/loading-pixel.gif");
    m_loader->setGeometry(rect());
    m_loader->hide();
    loadNextLevel();

    if (m_player)
        centerOn(m_player);

    // movement timer: ~60 FPS
    connect(&m_moveTimer, &QTimer::timeout, this, &GameView::stepMovement);
    m_moveTimer.setInterval(16); // ms (approx 60 FPS)
    m_moveTimer.start();

    // Monster AI updates every 30ms (~33 FPS), and also refreshes player HP bar
    connect(&m_monsterAITimer, &QTimer::timeout, this, [this]() {
        if (!m_player)
            return;
        updateMonsters();      // Monster pathfinding + attacks
        updatePlayerHpBar();   // HP bar follows player
        ++m_tickCount;         // Controls monster attack timing
    });
    m_monsterAITimer.setInterval(30);
    m_monsterAITimer.start();
}

void GameView::loadNextLevel()
{
    if (m_isLoading)
        return;

    m_isLoading = true;

    // Stop monster AI to avoid accessing deleted monsters
    m_monsterAITimer.stop();

    // Show GIF overlay
    if (m_loader) {
        m_loader->showOverlay();
    }

    QApplication::processEvents();

    QTimer::singleShot(500, this, &GameView::finishLoadNextLevel);
}

void GameView::finishLoadNextLevel()
{
    buildMaze();

    if (m_loader) {
        m_loader->hideOverlay();
    }

    if (m_player) {
        centerOn(m_player);
    }

    // Restart monster AI
    m_monsterAITimer.start();

    m_isLoading = false;
}

void GameView::buildMaze()
{
    m_scene->clear();
    m_player   = nullptr;
    m_exitTile = nullptr;
    m_doors.clear();

    // ---- Reset state (scene already cleared all items) ----
    m_monsters.clear();       // Only clear container; do not manually delete
    m_walkableCells.clear();
    m_monstersSpawned = false;
    m_playerHp        = m_playerMaxHp;
    m_playerSlowed    = false;
    m_tickCount       = 0;

    QString base = QCoreApplication::applicationDirPath();
    m_wallSet  = loadRandomTextureSet(wallFamilies(),  m_cellSize,
                                     base + "/texture/walls/");
    m_floorSet = loadRandomTextureSet(floorFamilies(), m_cellSize,
                                      base + "/texture/floor/");

    // Generate maze data (grid + start + exit + doors + keys)
    MazeGenerator gen(m_rowsCells, m_colsCells);
    MazeGenerator::MazeData maze = gen.generate();

    // Save a copy of maze grid for monster collision (1 = wall)
    m_grid = maze.grid;

    const auto &grid = maze.grid;
    int gridRows = static_cast<int>(grid.size());
    int gridCols = static_cast<int>(grid[0].size());

    int coarseRows = 2 * m_rowsCells + 1;
    int cellScale = gridRows / coarseRows;

    // Scene size in pixels
    int sceneWidth  = gridCols * m_cellSize;
    int sceneHeight = gridRows * m_cellSize;
    m_scene->setSceneRect(0, 0, sceneWidth, sceneHeight);
    setScene(m_scene);

    // --- Draw floor tiles for all walkable cells ---
    for (int r = 0; r < gridRows; ++r) {
        for (int c = 0; c < gridCols; ++c) {
            if (grid[r][c] == 1)
                continue;

            // Record all "non-wall" cells; later used for random monster spawn
            m_walkableCells.append(QPoint(c, r));

            int x = c * m_cellSize;
            int y = r * m_cellSize;

            QBrush brush = m_floorSet.randomBrush();
            if (brush.texture().isNull())
                continue;

            auto *floor = m_scene->addRect(
                x, y,
                m_cellSize, m_cellSize,
                QPen(Qt::NoPen),
                brush
                );
            floor->setZValue(-2);  // behind everything
            floor->setData(0, "floor");
        }
    }

    // Draw walls only
    for (int r = 0; r < gridRows; ++r) {
        for (int c = 0; c < gridCols; ++c) {
            if (grid[r][c] != 1)
                continue;

            int x = c * m_cellSize;
            int y = r * m_cellSize;

            QBrush brush = m_wallSet.randomBrush();
            if (brush.texture().isNull())
                continue;

            auto *wall = m_scene->addRect(
                x, y,
                m_cellSize, m_cellSize,
                QPen(Qt::NoPen),
                brush
                );
            wall->setData(0, "wall");
            wall->setZValue(-1);  // above floor, below keys/player
        }
    }

    // Draw EXIT (2x2 block)
    {
        int exitX = maze.exit.c * m_cellSize;
        int exitY = maze.exit.r * m_cellSize;

        QString exitPath = base + "/texture/exit/stone_stairs_down.png";
        QPixmap exitTex(exitPath);

        int exitW = cellScale * m_cellSize;
        int exitH = cellScale * m_cellSize;

        if (!exitTex.isNull()) {
            QPixmap scaledExit = exitTex.scaled(
                exitW, exitH,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
                );

            auto *exitItem = m_scene->addPixmap(scaledExit);
            exitItem->setPos(exitX, exitY);
            exitItem->setData(0, "exit");
            exitItem->setZValue(-0.5);

            m_exitTile = exitItem;
        }
    }

    // Draw doors
    for (int i = 0; i < static_cast<int>(maze.doors.size()); ++i) {
        MazeGenerator::Cell d = maze.doors[i];
        int doorX = d.c * m_cellSize;
        int doorY = d.r * m_cellSize;

        QPixmap doorTex(base + "/texture/doors/closed/dngn_closed_door.png");

        int doorW = cellScale * m_cellSize;
        int doorH = cellScale * m_cellSize;

        QPixmap scaledDoor = doorTex.scaled(
            doorW, doorH,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            );

        auto *doorItem = m_scene->addPixmap(scaledDoor);
        doorItem->setPos(doorX, doorY);
        doorItem->setData(0, "door");
        doorItem->setData(1, false);   // locked
        doorItem->setZValue(0);
        m_doors.push_back(doorItem);
    }

    // Draw keys
    for (int i = 0; i < static_cast<int>(maze.keys.size()); ++i) {
        MazeGenerator::Cell k = maze.keys[i];
        QPixmap keyTex(base + "/texture/keys/key.png");
        if (!keyTex.isNull())
        {
            int keyWidth  = m_cellSize * 0.8;
            int keyHeight = m_cellSize * 1.2;

            QPixmap scaledKey = keyTex.scaled(
                keyWidth, keyHeight,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                );

            int keyX = k.c * m_cellSize + (m_cellSize - scaledKey.width()) / 2;
            int keyY = k.r * m_cellSize + (m_cellSize - scaledKey.height()) / 2;

            auto *keyItem = m_scene->addPixmap(scaledKey);
            keyItem->setPos(keyX, keyY);
            keyItem->setData(0, "key");
            keyItem->setData(1, i);       // door index
            keyItem->setZValue(1);
        }
    }

    // Create player
    QString chosenCharacter = m_characterName;
    QDir appDir(QCoreApplication::applicationDirPath());
    QString spriteRoot = appDir.filePath("characters/" + chosenCharacter);

    m_player = new PlayerItem(spriteRoot, m_cellSize);
    m_scene->addItem(m_player);

    // ---- Create HP bar above the player's head ----
    m_playerHpBg = new QGraphicsRectItem();
    m_playerHpFg = new QGraphicsRectItem();

    m_playerHpBg->setBrush(Qt::red);
    m_playerHpBg->setPen(Qt::NoPen);
    m_playerHpFg->setBrush(Qt::green);
    m_playerHpFg->setPen(Qt::NoPen);

    m_playerHpBg->setZValue(9999);
    m_playerHpFg->setZValue(10000);

    m_scene->addItem(m_playerHpBg);
    m_scene->addItem(m_playerHpFg);

    updatePlayerHpBar();

    // Position player at start cell
    qreal px = maze.start.c * m_cellSize + m_cellSize / 2.0;
    qreal py = maze.start.r * m_cellSize + m_cellSize / 2.0;

    QRectF b = m_player->boundingRect();
    m_player->setPos(px - b.width() / 2.0, py - b.height() / 2.0);

    // Step size for movement
    m_step = 7;
    m_controller.setPlayer(m_player);
    m_controller.setStep(m_step);
#if CONTROL == GPIO
    m_controller.setGpios(67, 68, 44, 26, 46);

    connect(&m_controller,
            &GpioController::attackTriggered,
            this,
            &GameView::resolvePlayerAttack);
#endif
}

void GameView::updatePlayerHpBar()
{
    if (!m_player || !m_playerHpBg || !m_playerHpFg)
        return;

    QRectF pb = m_player->boundingRect();

    // Center-top of player (scene coordinates)
    QPointF topCenter = m_player->mapToScene(
        QPointF(pb.center().x(), pb.top())
        );

    qreal w = m_cellSize * 1.0;
    qreal h = 6.0;

    QPointF pos(topCenter.x() - w/2.0, topCenter.y() - h - 8);

    m_playerHpBg->setRect(pos.x(), pos.y(), w, h);

    qreal ratio = qMax(0.0, (double)m_playerHp / m_playerMaxHp);
    m_playerHpFg->setRect(pos.x(), pos.y(), w * ratio, h);
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    if (!m_player) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    bool handled = m_controller.handleKeyPress(event);
    if (!handled) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    // If attack key (space), immediately resolve melee attack
    if (event->key() == Qt::Key_Space) {
        resolvePlayerAttack();
    }

    if (m_controller.isMoving() && !m_moveTimer.isActive())
        m_moveTimer.start();
}

void GameView::keyReleaseEvent(QKeyEvent *event)
{
    if (!m_player) {
        QGraphicsView::keyReleaseEvent(event);
        return;
    }

    bool handled = m_controller.handleKeyRelease(event);
    if (!handled) {
        QGraphicsView::keyReleaseEvent(event);
        return;
    }

    if (!m_controller.isMoving()) {
        m_moveTimer.stop();
    }
}

void GameView::stepMovement()
{
    if (!m_player)
        return;

    QPointF delta = m_controller.movementDelta();
    if (delta.isNull())
        return;

    // Only process player movement; monster AI runs in separate timer
    tryMovePlayer(delta);
    updatePlayerHpBar();
}

bool GameView::tryMovePlayer(const QPointF &delta)
{
    if (!m_player) return false;

    QPointF oldPos = m_player->pos();
    QPointF newPos = oldPos + delta;

    m_player->setPos(newPos);

    QRectF spriteLocal = m_player->boundingRect();
    QPointF bottomCenterLocal(
        spriteLocal.center().x(),
        spriteLocal.bottom()
        );
    QPointF bottomCenterScene = m_player->mapToScene(bottomCenterLocal);

    // ---- Feet collision box ----
    qreal hitHeight = m_cellSize * 0.6;
    qreal hitWidth  = m_cellSize * 0.6;

    QRectF feetScene(
        bottomCenterScene.x() - hitWidth / 2.0,
        bottomCenterScene.y() - hitHeight - 13.0,
        hitWidth,
        hitHeight
        );

    QPainterPath feetPath;
    feetPath.addRect(feetScene);

    QList<QGraphicsItem*> items = m_scene->items(
        feetPath,
        Qt::IntersectsItemShape,
        Qt::DescendingOrder,
        QTransform()
        );

    bool blocked     = false;
    bool reachedExit = false;

    for (QGraphicsItem *item : items) {
        if (item == m_player) continue;

        QVariant tag = item->data(0);

        if (tag == "wall") {
            blocked = true;
        } else if (tag == "door") {
            bool unlocked = item->data(1).toBool();
            if (!unlocked) blocked = true;
        } else if (tag == "key") {
            int doorIndex = item->data(1).toInt();

            if (doorIndex >= 0 && doorIndex < static_cast<int>(m_doors.size())) {

                QGraphicsPixmapItem *door = m_doors[doorIndex];
                door->setData(1, true);  // unlocked

                QString base = QCoreApplication::applicationDirPath();
                QString openPath = base + "/texture/doors/open/dngn_open_door.png";
                QPixmap openTex(openPath);

                if (!openTex.isNull()) {
                    openTex = openTex.scaled(
                        door->boundingRect().width(),
                        door->boundingRect().height(),
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation
                        );
                    door->setPixmap(openTex);
                }
            }

            spawnMonsters(4);
            m_scene->removeItem(item);
            delete item;
        } else if (tag == "exit") {
            reachedExit = true;
        }
    }

    if (blocked) {
        m_player->setPos(oldPos);
        return false;
    }

    centerOn(m_player);

    if (reachedExit) {
        for (MonsterItem *m : m_monsters) {
            if (m) {
                m_scene->removeItem(m);
                delete m;
            }
        }
        m_monsters.clear();

        loadNextLevel();
        viewport()->update();
    }

    return true;
}

void GameView::spawnMonsters(int count)
{
    if (m_walkableCells.isEmpty())
        return;

    QString base = QCoreApplication::applicationDirPath();

    // Two kinds of monster textures
    QPixmap damagePix(base + "/monsters/damage.png"); // Damage monster
    QPixmap slowPix(base + "/monsters/slow.png");     // Slow monster

    if (damagePix.isNull() || slowPix.isNull()) {
        qWarning() << "[GameView] Monster textures not found under"
                   << base + "/monsters";
    }

    for (int i = 0; i < count; ++i) {
        int idx = QRandomGenerator::global()->bounded(m_walkableCells.size());
        QPoint cell = m_walkableCells[idx];

        QPointF center(cell.x() * m_cellSize + m_cellSize / 2.0,
                       cell.y() * m_cellSize + m_cellSize / 2.0);

        int r = QRandomGenerator::global()->bounded(100);
        MonsterItem::MonsterType t =
            (r < 30 ? MonsterItem::DamageMonster   // 30% damage monster
                    : MonsterItem::SlowMonster);    // 70% slow monster

        const QPixmap &pix = (t == MonsterItem::DamageMonster ? damagePix : slowPix);
        if (pix.isNull())
            continue;

        MonsterItem *monster = new MonsterItem(t, pix, 100, m_cellSize);
        m_scene->addItem(monster);

        QRectF b = monster->boundingRect();
        monster->setPos(center.x() - b.width() / 2.0,
                        center.y() - b.height() / 2.0);

        m_monsters.append(monster);
    }
}

void GameView::updateMonsters()
{
    if (!m_player || m_monsters.isEmpty())
        return;

    QRectF pb = m_player->boundingRect();
    QPointF playerCenter = m_player->pos() + QPointF(pb.width() / 2.0,
                                                     pb.height());

    qreal attackRadius = m_cellSize * 1.0;

    // 30ms * 15 ≈ 450ms ≈ 0.5s per attack
    const int ticksPerHit = 15;

    for (int i = m_monsters.size() - 1; i >= 0; --i) {
        MonsterItem* m = m_monsters[i];
        if (!m) continue;

        if (m->isDead()) {
            m_scene->removeItem(m);
            delete m;
            m_lastAttackTick.remove(m);
            m_touchingMonsters.remove(m);
            m_monsters.removeAt(i);
            continue;
        }

        QRectF mb = m->boundingRect();
        QPointF monsterCenter = m->pos() + QPointF(mb.width()/2.0,
                                                   mb.height());

        QLineF line(monsterCenter, playerCenter);
        qreal dist = line.length();

        bool touching = (dist <= attackRadius);

        // ---- Attack: when touching, damage approx every 0.5s ----
        if (touching) {
            int lastTick = m_lastAttackTick.value(m, -1000000);
            if (m_tickCount - lastTick >= ticksPerHit) {
                if (m->monsterType() == MonsterItem::DamageMonster)
                    damagePlayer(10);
                else
                    applySlowToPlayer(1500, 0.5);  // 1.5s slow to 50%

                m_lastAttackTick[m] = m_tickCount;
            }

            m_touchingMonsters.insert(m);
            // When touching, monster stays still or moves minimally; do not chase
            continue;
        } else {
            // Leaving contact resets cooldown
            m_touchingMonsters.remove(m);
        }

        // ---- Random decision: chase player or wander ----
        bool chase = (QRandomGenerator::global()->bounded(100) < 85);

        QPointF delta(0, 0);
        qreal step = m->speed();

        if (chase && dist > 0.1) {
            // Normal chase
            line.setLength(step);
            delta = QPointF(line.dx(), line.dy());
        } else {
            // Random direction
            int angleDeg = QRandomGenerator::global()->bounded(360);
            qreal rad = angleDeg * (3.14159265 / 180.0);
            delta = QPointF(std::cos(rad) * step,
                            std::sin(rad) * step);
        }

        // ---- Small noisy jitter (to avoid synchronized movement) ----
        qreal noiseX = (QRandomGenerator::global()->bounded(100) - 50) / 200.0;
        qreal noiseY = (QRandomGenerator::global()->bounded(100) - 50) / 200.0;
        delta += QPointF(noiseX, noiseY);

        // ---- Repulsion (prevent crowding) ----
        for (MonsterItem* other : m_monsters) {
            if (other == m) continue;

            QRectF ob = other->boundingRect();
            QPointF otherCenter = other->pos() + QPointF(ob.width()/2.0,
                                                         ob.height()/2.0);

            qreal d = QLineF(monsterCenter, otherCenter).length();
            if (d < (m_cellSize * 1.1) && d > 0.01) {
                QPointF diff = monsterCenter - otherCenter;
                diff /= d;
                delta += diff * 0.4;   // strength of repulsion
            }
        }

        m->setPos(m->pos() + delta);
    }

    // Remove attack records of monsters that no longer exist
    for (auto it = m_lastAttackTick.begin(); it != m_lastAttackTick.end(); ) {
        if (!m_monsters.contains(it.key()))
            it = m_lastAttackTick.erase(it);
        else
            ++it;
    }
}

void GameView::damagePlayer(int amount)
{
    m_playerHp -= amount;
    if (m_playerHp < 0)
        m_playerHp = 0;

    qDebug() << "[GameView] Player HP:" << m_playerHp << "/" << m_playerMaxHp;

    if (m_playerHp <= 0) {
        // Simple handling: stop movement & play death action
        m_moveTimer.stop();
        m_monsterAITimer.stop();
        m_controller.setStep(0);
        m_player->setAction(PlayerItem::Dying);
        QTimer::singleShot(6000, this, [](){
            QApplication::quit();  // Quit whole program
        });
    }
}

void GameView::applySlowToPlayer(int durationMs, qreal factor)
{
    if (m_playerSlowed)
        return;

    m_playerSlowed = true;

    int originalStep = m_step;
    int slowedStep   = qMax(1, int(originalStep * factor));
    m_controller.setStep(slowedStep);

    // Restore speed after duration
    QTimer::singleShot(durationMs, this, [this, originalStep]() {
        m_controller.setStep(originalStep);
        m_playerSlowed = false;
    });
}

void GameView::resolvePlayerAttack()
{
    if (!m_player)
        return;

    QRectF spriteLocal = m_player->boundingRect();
    QPointF bottomCenterLocal(
        spriteLocal.center().x(),
        spriteLocal.bottom()
        );
    QPointF bottomCenterScene = m_player->mapToScene(bottomCenterLocal);

    QRectF attackRect;
    qreal w = m_cellSize;
    qreal h = m_cellSize;

    switch (m_player->direction()) {
    case PlayerItem::Front:
        attackRect = QRectF(bottomCenterScene.x() - w/2.0,
                            bottomCenterScene.y(),
                            w, h);
        break;
    case PlayerItem::Back:
        attackRect = QRectF(bottomCenterScene.x() - w/2.0,
                            bottomCenterScene.y() - h,
                            w, h);
        break;
    case PlayerItem::Left:
        attackRect = QRectF(bottomCenterScene.x() - w,
                            bottomCenterScene.y() - h/2.0,
                            w, h);
        break;
    case PlayerItem::Right:
        attackRect = QRectF(bottomCenterScene.x(),
                            bottomCenterScene.y() - h/2.0,
                            w, h);
        break;
    }

    QPainterPath path;
    path.addRect(attackRect);

    QList<QGraphicsItem*> items = m_scene->items(
        path,
        Qt::IntersectsItemShape,
        Qt::DescendingOrder,
        QTransform()
        );

    // Attack only the closest monster
    MonsterItem* closest = nullptr;
    qreal bestDist = 1e9;

    QPointF playerCenter = m_player->mapToScene(spriteLocal.center());

    for (QGraphicsItem *item : items) {
        auto *monster = dynamic_cast<MonsterItem*>(item);
        if (monster) {
            QRectF mb = monster->boundingRect();
            QPointF mc = monster->pos() + QPointF(mb.width()/2.0, mb.height()/2.0);

            qreal d = QLineF(playerCenter, mc).length();
            if (d < bestDist) {
                bestDist = d;
                closest = monster;
            }
        }
    }

    if (closest) {
        closest->takeDamage(40);
    }
}

// ---- Monster collision check: is the target position a wall? ----
bool GameView::monsterCanMoveTo(const QPointF &pos)
{
    if (m_grid.empty())
        return false;

    int col = static_cast<int>(pos.x() / m_cellSize);
    int row = static_cast<int>(pos.y() / m_cellSize);

    if (row < 0 || col < 0)
        return false;

    int rows = static_cast<int>(m_grid.size());
    int cols = static_cast<int>(m_grid[0].size());

    if (row >= rows || col >= cols)
        return false;

    // 1 means wall; non-wall is walkable
    return (m_grid[row][col] != 1);
}

void GameView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    if (m_loader) {
        m_loader->setGeometry(rect());
    }
}
