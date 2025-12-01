#ifndef PLAYERITEM_H
#define PLAYERITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QMap>
#include <QVector>
#include <QPixmap>
#include <QTimer>

class PlayerItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    enum Direction { Front, Back, Left, Right };
    enum Action    { Idle, Walk, Run, Attack, Dying };

    explicit PlayerItem(const QString &spriteRoot,
                        int tileSize,
                        QGraphicsItem *parent = nullptr);

    void setDirection(Direction dir);
    void setAction(Action act);

    Direction direction() const { return m_direction; }
    Action    action()    const { return m_action;    }

private slots:
    void nextFrame();

private:
    struct Animation {
        QVector<QPixmap> frames;
        int fps = 8;
    };

    QMap<QString, Animation> m_animations;
    QString m_currentKey;
    int     m_frameIndex;
    QTimer  m_timer;
    QString m_spriteRoot;

    int m_tileSize;
    bool m_scaleComputed = false;
    float m_scale = 0.8f;

    Direction m_direction;
    Action    m_action;

    QVector<QPixmap> loadFrames(const QString &folder);
    void addAnimation(const QString &key,
                      const QString &folderOnDisk,
                      int fps);
    void setAnimation(const QString &key);
    QString keyFor(Direction d, Action a) const;
};

#endif // PLAYERITEM_H
