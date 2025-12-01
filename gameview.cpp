#include "gameview.h"
#include "mazegenerator.h"
#include "playercontroller.h"

#include <QKeyEvent>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QDir>
#include <QApplication>

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
    loadNextLevel();

    if (m_player)
        centerOn(m_player);

    // movement timer: ~60 FPS
    connect(&m_moveTimer, &QTimer::timeout, this, &GameView::stepMovement);
    m_moveTimer.setInterval(16); // ms (approx 60 FPS)
    m_moveTimer.start();
}

void GameView::loadNextLevel()
{
    if (m_isLoading)
        return;

    m_isLoading = true;

    // Show GIF overlay
    if (m_loader) {
        m_loader->showOverlay();
    }

    // Let UI draw the GIF immediately
    QApplication::processEvents();

    // After a short delay, actually build the maze.
    QTimer::singleShot(500, this, &GameView::finishLoadNextLevel);
}

void GameView::finishLoadNextLevel()
{
    // Build a fresh maze (this will reset player, doors, keys, etc.)
    buildMaze();

    // Hide GIF overlay
    if (m_loader) {
        m_loader->hideOverlay();
    }

    if (m_player) {
        centerOn(m_player);
    }

    m_isLoading = false;
}

void GameView::buildMaze()
{
    m_scene->clear();
    m_player   = nullptr;
    m_exitTile = nullptr;
    m_doors.clear();

    QString base = QCoreApplication::applicationDirPath();
    m_wallSet  = loadRandomTextureSet(wallFamilies(),  m_cellSize,
                                     base + "/texture/walls/");
    m_floorSet = loadRandomTextureSet(floorFamilies(), m_cellSize,
                                      base + "/texture/floor/");

    // Generate maze data (grid + start + exit + doors + keys)
    MazeGenerator gen(m_rowsCells, m_colsCells);
    MazeGenerator::MazeData maze = gen.generate();

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
                continue; // wall, handled later

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
            floor->setZValue(-2);        // behind everything else
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

    // Draw EXIT (2x2 block around exit cell for visibility)
    {
        int exitX = maze.exit.c * m_cellSize;
        int exitY = maze.exit.r * m_cellSize;

        QString exitPath = base + "/texture/exit/stone_stairs_down.png";
        QPixmap exitTex(exitPath);

        int exitW = cellScale * m_cellSize;
        int exitH = cellScale * m_cellSize;

        if (!exitTex.isNull()) {
            // Scale stair texture to exactly fill the 2×2 exit region
            QPixmap scaledExit = exitTex.scaled(
                exitW,
                exitH,
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

    // Draw DOORS as 2x2 blocks using maze.doors
    for (int i = 0; i < static_cast<int>(maze.doors.size()); ++i) {
        MazeGenerator::Cell d = maze.doors[i];
        int doorX = d.c * m_cellSize;
        int doorY = d.r * m_cellSize;

        QPixmap doorTex(base + "/texture/doors/closed/dngn_closed_door.png");

        int doorW = cellScale * m_cellSize;
        int doorH = cellScale * m_cellSize;

        // Scale door so it fits 2x2 cells nicely
        QPixmap scaledDoor = doorTex.scaled(
            doorW,
            doorH,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            );

        auto *doorItem = m_scene->addPixmap(scaledDoor);
        doorItem->setPos(doorX, doorY);

        doorItem->setData(0, "door");
        doorItem->setData(1, false);
        doorItem->setZValue(0);
        m_doors.push_back(doorItem);
    }

    // Draw KEYS (one per door) using maze.keys
    for (int i = 0; i < static_cast<int>(maze.keys.size()); ++i) {
        MazeGenerator::Cell k = maze.keys[i];
        QPixmap keyTex(base + "/texture/keys/key.png");
        if (!keyTex.isNull())
        {
            int keyWidth  = m_cellSize * 0.8;   // 80% of tile width
            int keyHeight = m_cellSize * 1.2;   // slightly taller than tile

            // Scale the texture
            QPixmap scaledKey = keyTex.scaled(
                keyWidth,
                keyHeight,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                );

            // Center inside the cell
            int keyX = k.c * m_cellSize + (m_cellSize - scaledKey.width()) / 2;
            int keyY = k.r * m_cellSize + (m_cellSize - scaledKey.height()) / 2;

            auto *keyItem = m_scene->addPixmap(scaledKey);
            keyItem->setPos(keyX, keyY);
            keyItem->setData(0, "key");
            keyItem->setData(1, i);       // which door
            keyItem->setZValue(1);        // above everything
        }
    }

    // Create player at START
    QString chosenCharacter = m_characterName;
    QDir appDir(QCoreApplication::applicationDirPath());
    QString spriteRoot = appDir.filePath("characters/" + chosenCharacter);

    m_player = new PlayerItem(spriteRoot, m_cellSize);
    m_scene->addItem(m_player);

    // Position player so its center is in the start cell
    qreal px = maze.start.c * m_cellSize + m_cellSize / 2.0;
    qreal py = maze.start.r * m_cellSize + m_cellSize / 2.0;

    QRectF b = m_player->boundingRect();
    m_player->setPos(px - b.width() / 2.0, py - b.height() / 2.0);

    // Step size for movement
    m_step = 7;
    m_controller.setPlayer(m_player);
    m_controller.setStep(m_step);
#if CONTROL == GPIO
    m_controller.setGpios(67, 68, 44, 26);
#endif
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

    // Start timer when movement begins
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

    // Stop timer when no direction is held
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

    tryMovePlayer(delta);
}

bool GameView::tryMovePlayer(const QPointF &delta)
{
    if (!m_player) return false;

    QPointF oldPos = m_player->pos();
    QPointF newPos = oldPos + delta;

    m_player->setPos(newPos);

    // 1-tile feet collider, independent of sprite height
    QRectF spriteLocal = m_player->boundingRect();
    QPointF bottomCenterLocal(
        spriteLocal.center().x(),
        spriteLocal.bottom()
        );
    QPointF bottomCenterScene = m_player->mapToScene(bottomCenterLocal);

    // --- FEET HITBOX: small box at the bottom ---
    qreal hitHeight = m_cellSize * 0.6;   // 60% of a tile height
    qreal hitWidth  = m_cellSize * 0.6;    // a bit narrower than a tile

    QRectF feetScene(
        bottomCenterScene.x() - hitWidth / 2.0,   // center horizontally
        bottomCenterScene.y() - hitHeight - 13.0,        // just above the feet
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

            // Valid door index?
            if (doorIndex >= 0 && doorIndex < static_cast<int>(m_doors.size())) {

                QGraphicsPixmapItem *door = m_doors[doorIndex];
                door->setData(1, true);        // mark unlocked

                // Load open door texture
                QString base = QCoreApplication::applicationDirPath();
                QString openPath = base + "/texture/doors/open/dngn_open_door.png";
                QPixmap openTex(openPath);

                if (!openTex.isNull()) {
                    // Scale open door image to match original size
                    openTex = openTex.scaled(
                        door->boundingRect().width(),
                        door->boundingRect().height(),
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation
                        );
                    door->setPixmap(openTex);
                }
            }

            // Remove the key from scene
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
        loadNextLevel();
        viewport()->update();
    }

    return true;
}
void GameView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    if (m_loader) {
        m_loader->setGeometry(rect());
    }
}
