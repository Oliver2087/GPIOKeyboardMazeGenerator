#include "playeritem.h"
#include <QDir>
#include <QDebug>

PlayerItem::PlayerItem(const QString &spriteRoot,
                       int tileSize,
                       QGraphicsItem *parent)
    : QObject(),
    QGraphicsPixmapItem(parent),
    m_spriteRoot(spriteRoot),
    m_tileSize(tileSize),
    m_scaleComputed(false),
    m_direction(Front),
    m_action(Idle)
{
    // Centered transform origin (useful if you add rotations later)
    setTransformOriginPoint(boundingRect().center());

    // ---- Load all animations from disk (NOT resources) ----
    addAnimation("front_idle",   m_spriteRoot + "/Front_Idle",      6);
    addAnimation("front_walk",   m_spriteRoot + "/Front_Walking",   10);
    addAnimation("front_run",    m_spriteRoot + "/Front_Running",   14);
    addAnimation("front_attack", m_spriteRoot + "/Front_Attacking", 12);

    addAnimation("back_idle",    m_spriteRoot + "/Back_Idle",       6);
    addAnimation("back_walk",    m_spriteRoot + "/Back_Walking",    10);
    addAnimation("back_run",     m_spriteRoot + "/Back_Running",    11);
    addAnimation("back_attack",  m_spriteRoot + "/Back_Attacking",  12);

    addAnimation("left_idle",    m_spriteRoot + "/Left_Idle",       6);
    addAnimation("left_walk",    m_spriteRoot + "/Left_Walking",    10);
    addAnimation("left_run",     m_spriteRoot + "/Left_Running",    14);
    addAnimation("left_attack",  m_spriteRoot + "/Left_Attacking",  12);

    addAnimation("right_idle",   m_spriteRoot + "/Right_Idle",      6);
    addAnimation("right_walk",   m_spriteRoot + "/Right_Walking",   10);
    addAnimation("right_run",    m_spriteRoot + "/Right_Running",   14);
    addAnimation("right_attack", m_spriteRoot + "/Right_Attacking", 12);

    addAnimation("dying",        m_spriteRoot + "/Dying",           8);

    // Default: front idle
    setAnimation("front_idle");

    connect(&m_timer, &QTimer::timeout, this, &PlayerItem::nextFrame);
}

QVector<QPixmap> PlayerItem::loadFrames(const QString &folder)
{
    QVector<QPixmap> frames;

    QDir dir(folder);
    if (!dir.exists()) {
        qWarning() << "[PlayerItem] Folder does not exist:" << folder;
        return frames;
    }

    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dir.setNameFilters(QStringList() << "*.png" << "*.PNG");
    dir.setSorting(QDir::Name | QDir::IgnoreCase);

    QStringList files = dir.entryList();
    for (const QString &file : files) {
        QString path = folder + "/" + file;
        QPixmap pix(path);
        if (pix.isNull()) {
            qWarning() << "[PlayerItem] Could not load frame:" << path;
            continue;
        }
        if (!m_scaleComputed) {

            int originalWidth = pix.width();   // width of first frame

            if (originalWidth > 0) {
                m_scale = (float)m_tileSize / (float)originalWidth;
                m_scaleComputed = true;
            }
        }
        if (m_scale != 1.0f) {
            int newW = pix.width()  * m_scale * 2;
            int newH = pix.height() * m_scale * 2;

            pix = pix.scaled(newW,
                             newH,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
        }

        frames.push_back(pix);
    }

    return frames;
}

void PlayerItem::addAnimation(const QString &key,
                              const QString &folderOnDisk,
                              int fps)
{
    Animation anim;
    anim.frames = loadFrames(folderOnDisk);
    anim.fps    = fps;

    if (!anim.frames.isEmpty())
        m_animations.insert(key, anim);
}

QString PlayerItem::keyFor(Direction d, Action a) const
{
    QString dirStr;
    switch (d) {
    case Front: dirStr = "front"; break;
    case Back:  dirStr = "back";  break;
    case Left:  dirStr = "left";  break;
    case Right: dirStr = "right"; break;
    }

    if (a == Dying)
        return "dying";

    QString actStr;
    switch (a) {
    case Idle:   actStr = "idle";   break;
    case Walk:   actStr = "walk";   break;
    case Run:    actStr = "run";    break;
    case Attack: actStr = "attack"; break;
    case Dying:  actStr = "dying";  break;
    }

    return dirStr + "_" + actStr; // e.g. "front_walk"
}

void PlayerItem::setAnimation(const QString &key)
{
    if (!m_animations.contains(key)) {
        qWarning() << "[PlayerItem] Unknown animation key:" << key;
        return;
    }

    m_currentKey = key;
    m_frameIndex = 0;

    Animation &anim = m_animations[m_currentKey];
    if (!anim.frames.isEmpty()) {
        setPixmap(anim.frames[0]);

        if (anim.fps > 0) {
            // Global speed tweak: change 1000 to 500 for 2x speed, etc.
            m_timer.start(1000 / anim.fps);
        } else {
            m_timer.stop();
        }
    } else {
        m_timer.stop();
    }
}

void PlayerItem::setDirection(Direction dir)
{
    if (m_direction == dir)
        return;

    m_direction = dir;
    setAnimation(keyFor(m_direction, m_action));
}

void PlayerItem::setAction(Action act)
{
    if (m_action == act)
        return;

    m_action = act;
    setAnimation(keyFor(m_direction, m_action));
}

void PlayerItem::nextFrame()
{
    if (m_currentKey.isEmpty())
        return;

    Animation &anim = m_animations[m_currentKey];
    if (anim.frames.isEmpty())
        return;

    m_frameIndex++;

    if (m_action == Attack) {
        // Attack plays once: stop at last frame
        if (m_frameIndex >= anim.frames.size()) {
            m_frameIndex = anim.frames.size() - 1;
            setPixmap(anim.frames[m_frameIndex]);
            m_timer.stop();

            // return to Idle after attack finishes
            m_action = Idle;
            setAnimation(keyFor(m_direction, m_action));
            return;
        }
    } else {
        // Normal looping animations
        m_frameIndex %= anim.frames.size();
    }

    setPixmap(anim.frames[m_frameIndex]);
}
