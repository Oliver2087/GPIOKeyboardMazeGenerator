#include "textures.h"
#include <QRandomGenerator>

QVector<TextureFamily> wallFamilies()
{
    return {
        { "brick_brown", 8 },          // brick_brown0..7
        { "brick_brown-vines", 4 },    // brick_brown-vines0..3
        { "brick_dark", 6 },           // brick_dark0..5
        { "brick_gray", 4 },           // brick_gray0..3
        { "crystal_wall", 14 },        // crystal_wall00..13
        { "hive", 4 },                 // hive0..3
        { "lair", 4 },                 // lair0..3
        { "marble_wall", 12 },         // marble_wall0..11
        { "pebble_red", 4 },           // pebble_red0..3
        { "relief", 4 },               // relief0..3
        { "sandstone_wall", 13 },      // sandstone_wall0..12
        { "slime", 4 },                // slime0..3
        { "stone_brick", 10 },         // stone_brick0..9
        { "stone_dark", 4 },           // stone_dark0..3
        { "stone_gray", 4 },           // stone_gray0..3
        { "stone2_gray", 4 },          // stone2_gray0..3
        { "stone2_brown", 4 },         // stone2_brown0..3
        { "stone2_dark", 4 },          // stone2_dark0..3
        { "tomb", 4 },                 // tomb0..3
        { "undead", 4 },               // undead0..3
        { "vault", 4 },                // vault0..3
        { "volcanic_wall", 6 },        // volcanic_wall0..5
        { "wall_vines", 5 },           // wall_vines0..4
        { "wall_yellow_rock", 4 }      // wall_yellow_rock0..3
    };
}

QVector<TextureFamily> floorFamilies()
{
    return {
        { "bog_green",        4 },
        { "cobble_blood",    13 },
        { "crystal_floor",    6 },
        { "dirt",             3 },
        { "floor_nerves",     7 },
        { "floor_sand_stone", 8 },
        { "floor_vines",      6 },
        { "grey_dirt",        8 },
        { "hive",             4 },
        { "ice",              4 },
        { "lair",             4 },
        { "marble_floor",     6 },
        { "mesh",             4 },
        { "pebble_brown",     8 },
        { "rect_gray",        3 },
        { "rough_red",        4 },
        { "sandstone_floor", 10 },
        { "snake",            4 },
        { "swamp",            4 },
        { "tomb",             4 },
        { "volcanic_floor",   7 }
    };
}

TextureSet loadRandomTextureSet(const QVector<TextureFamily> &families,
                                int cellSize,
                                const QString &basePath)
{
    TextureSet set;

    if (families.isEmpty())
        return set;

    int idx = QRandomGenerator::global()->bounded(families.size());
    const TextureFamily &chosen = families[idx];
    set.prefix = chosen.prefix;

    for (int i = 0; i < chosen.count; ++i) {
        // If you are using Qt resources (.qrc), basePath should be like
        // ":/texture/walls/" or ":/texture/floor/"
        QString fileName = QString("%1%2%3.png")
                               .arg(basePath)
                               .arg(chosen.prefix)
                               .arg(i);

        QPixmap tex(fileName);
        if (tex.isNull())
            continue;

        tex = tex.scaled(cellSize, cellSize,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation);
        set.pixmaps.push_back(tex);
        set.brushes.push_back(QBrush(set.pixmaps.back()));
    }

    return set;
}

QBrush TextureSet::randomBrush() const
{
    if (brushes.isEmpty())
        return QBrush();
    int idx = QRandomGenerator::global()->bounded(brushes.size());
    return brushes[idx];
}
