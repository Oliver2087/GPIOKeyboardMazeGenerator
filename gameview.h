#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QPixmap>
#include <QBrush>
#include <QFutureWatcher>
#include <vector>

#include "playeritem.h"
#include "playercontroller.h"
#include "gpiocontroller.h"
#include "textures.h"
#include "loadingoverlay.h"
#include "mazegenerator.h"

class QKeyEvent;
class QResizeEvent;

#define KEY     1
#define GPIO    2
#define CONTROL GPIO

struct WallStyle {
    QString prefix;   // e.g. "brick_brown" or "stone_brick"
    int count;        // how many numbered variants: 0..count-1
};

struct FloorStyle {
    QString prefix;   // e.g. "brick_brown" or "stone_brick"
    int count;        // how many numbered variants: 0..count-1
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
    void stepMovement();  // timer callback
    void onMazeGenerated();

private:
    void buildMaze(const MazeGenerator::MazeData &maze);
    bool tryMovePlayer(const QPointF &delta); // movement + collision

    QGraphicsScene     *m_scene;
    PlayerItem         *m_player;
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

    QTimer m_moveTimer;
#if CONTROL==GPIO
    GpioController m_controller;
#else
    PlayerController m_controller;
#endif

    QFutureWatcher<MazeGenerator::MazeData> m_mazeWatcher;
};

#endif // GAMEVIEW_H
