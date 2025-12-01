#ifndef TEXTURES_H
#define TEXTURES_H

#include <QVector>
#include <QString>
#include <QPixmap>
#include <QBrush>

struct TextureFamily
{
    QString prefix;   // e.g. "brick_brown"
    int     count;    // how many numbered tiles (0..count-1)
};

struct TextureSet
{
    QString          prefix;
    QVector<QPixmap> pixmaps;
    QVector<QBrush>  brushes;

    QBrush randomBrush() const;
};

// Lists of available families
QVector<TextureFamily> wallFamilies();
QVector<TextureFamily> floorFamilies();

// Pick a random family from list & load all its textures
TextureSet loadRandomTextureSet(const QVector<TextureFamily> &families,
                                int cellSize,
                                const QString &basePath);

#endif // TEXTURES_H
