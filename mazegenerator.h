#ifndef MAZEGENERATOR_H
#define MAZEGENERATOR_H

#include <vector>

class MazeGenerator {
public:
    struct Cell {
        int r;
        int c;
    };

    struct MazeData {
        std::vector<std::vector<int>> grid;  // 0=passage, 1=wall, 2=door
        Cell start;
        Cell exit;
        std::vector<Cell> doors;             // door positions (fine grid, top-left of 2x2)
        std::vector<Cell> keys;              // key positions (fine grid)
    };

    MazeGenerator(int rows, int cols);

    MazeData generate();

private:
    void carveFrom(int r, int c);
    void widenGrid(int scale);
    Cell pickFarthestCell(const Cell &from) const;
    std::vector<Cell> shortestPath(const Cell &start, const Cell &goal) const;

    int m_rowsCells;
    int m_colsCells;

    int m_gridRows;
    int m_gridCols;
    std::vector<std::vector<int>> m_grid;

    int m_exitRowCoarse;
    int m_exitColCoarse;
};

#endif // MAZEGENERATOR_H
