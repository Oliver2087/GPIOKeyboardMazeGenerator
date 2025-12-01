#include "mazegenerator.h"
#include <QRandomGenerator>
#include <queue>
#include <algorithm>

MazeGenerator::MazeGenerator(int rows, int cols)
    : m_rowsCells(rows),
    m_colsCells(cols)
{
    // Coarse grid: odd cells are passages, even are walls
    m_gridRows = 2 * m_rowsCells + 1;
    m_gridCols = 2 * m_colsCells + 1;
    m_grid.assign(m_gridRows, std::vector<int>(m_gridCols, 1));
}

MazeGenerator::MazeData MazeGenerator::generate()
{
    // 1) carve coarse maze
    carveFrom(1, 1);
    const int scale = 2;

    // 2) pick exit on TOP of coarse maze (row 1, near left/right)
    m_exitRowCoarse = 1;
    bool exitOnLeft = (QRandomGenerator::global()->bounded(2) == 0);
    m_exitColCoarse = exitOnLeft ? 1 : (m_gridCols - 2);

    m_grid[m_exitRowCoarse][m_exitColCoarse] = 0;
    m_grid[0][m_exitColCoarse]               = 0;

    // 3) work on COARSE grid
    Cell exitCoarse  { m_exitRowCoarse, m_exitColCoarse };
    Cell startCoarse = pickFarthestCell(exitCoarse);
    std::vector<Cell> coarsePath = shortestPath(startCoarse, exitCoarse);

    std::vector<Cell> doorCoarse;
    std::vector<Cell> keyCoarse;

    int pathLen = static_cast<int>(coarsePath.size());
    if (pathLen > 4) {
        int maxDoors  = 3;
        int doorCount = std::min(maxDoors, pathLen / 6); // about 1 per 6 tiles

        // mark which cells are on the main path
        std::vector<std::vector<bool>> onPath(
            m_gridRows, std::vector<bool>(m_gridCols, false));
        for (const Cell &c : coarsePath) {
            onPath[c.r][c.c] = true;
        }

        auto inside = [&](int r, int c) {
            return r >= 0 && r < m_gridRows && c >= 0 && c < m_gridCols;
        };

        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};

        for (int i = 0; i < doorCount; ++i) {
            // ---- choose door on path ----
            int idxDoor = (i + 1) * pathLen / (doorCount + 1);
            if (idxDoor <= 1)           idxDoor = 2;
            if (idxDoor >= pathLen - 2) idxDoor = pathLen - 3;

            Cell dCell = coarsePath[idxDoor];
            doorCoarse.push_back(dCell);

            // ---- BFS from start, treating this door as a wall ----
            std::queue<Cell> q;
            std::vector<std::vector<int>> dist(
                m_gridRows, std::vector<int>(m_gridCols, -1));

            q.push(startCoarse);
            dist[startCoarse.r][startCoarse.c] = 0;

            bool haveOffPath = false;
            Cell bestOffPath = startCoarse;
            int  bestOffDist = 0;

            Cell bestAny     = startCoarse;
            int  bestAnyDist = 0;

            while (!q.empty()) {
                Cell cur = q.front();
                q.pop();
                int curDist = dist[cur.r][cur.c];

                // track globally farthest visited cell
                if (curDist > bestAnyDist) {
                    bestAnyDist = curDist;
                    bestAny     = cur;
                }
                // track farthest cell NOT on the main path
                if (!onPath[cur.r][cur.c] && curDist > bestOffDist) {
                    bestOffDist = curDist;
                    bestOffPath = cur;
                    haveOffPath = true;
                }

                for (int k = 0; k < 4; ++k) {
                    int nr = cur.r + dr[k];
                    int nc = cur.c + dc[k];

                    if (!inside(nr, nc))
                        continue;
                    if (m_grid[nr][nc] == 1)
                        continue;          // wall
                    if (nr == dCell.r && nc == dCell.c)
                        continue;          // treat door as blocked
                    if (dist[nr][nc] != -1)
                        continue;          // already visited

                    dist[nr][nc] = curDist + 1;
                    q.push({nr, nc});
                }
            }

            // key at farthest off-path cell if possible,
            // otherwise farthest reachable cell at all
            Cell keyCell = haveOffPath ? bestOffPath : bestAny;
            keyCoarse.push_back(keyCell);
        }
    }

    // 5) mark doors in the *coarse* grid as value 2
    for (const Cell &d : doorCoarse) {
        if (d.r >= 0 && d.r < m_gridRows &&
            d.c >= 0 && d.c < m_gridCols &&
            m_grid[d.r][d.c] == 0) {
            m_grid[d.r][d.c] = 2;
        }
    }

    // 6) widen maze: every coarse cell (0/1/2) -> 2x2 block in fine grid
    widenGrid(scale);

    // 7) convert coarse start/exit/doors/keys to fine coordinates (top-left of block)
    Cell startFine { startCoarse.r * scale, startCoarse.c * scale };
    Cell exitFine  { exitCoarse.r  * scale, exitCoarse.c  * scale };

    std::vector<Cell> doorFine;
    doorFine.reserve(doorCoarse.size());
    for (const Cell &d : doorCoarse) {
        doorFine.push_back({ d.r * scale, d.c * scale });
    }

    std::vector<Cell> keyFine;
    keyFine.reserve(keyCoarse.size());
    for (const Cell &k : keyCoarse) {
        keyFine.push_back({ k.r * scale, k.c * scale });
    }

    // 8) package result
    MazeData data;
    data.grid  = m_grid;
    data.start = startFine;
    data.exit  = exitFine;
    data.doors = doorFine;
    data.keys  = keyFine;
    return data;
}



void MazeGenerator::widenGrid(int scale)
{
    int newRows = m_gridRows * scale;
    int newCols = m_gridCols * scale;

    std::vector<std::vector<int>> newGrid(
        newRows, std::vector<int>(newCols, 1));

    for (int r = 0; r < m_gridRows; ++r) {
        for (int c = 0; c < m_gridCols; ++c) {
            int val = m_grid[r][c];
            for (int dr = 0; dr < scale; ++dr) {
                for (int dc = 0; dc < scale; ++dc) {
                    int nr = r * scale + dr;
                    int nc = c * scale + dc;
                    newGrid[nr][nc] = val;
                }
            }
        }
    }

    m_gridRows = newRows;
    m_gridCols = newCols;
    m_grid = std::move(newGrid);
}

void MazeGenerator::carveFrom(int r, int c)
{
    m_grid[r][c] = 0;

    struct Dir { int dr, dc; };
    std::vector<Dir> dirs = {
        { -2,  0 },
        {  2,  0 },
        {  0, -2 },
        {  0,  2 }
    };

    int n = int(dirs.size());
    for (int i = 0; i < n; ++i) {
        int j = QRandomGenerator::global()->bounded(i, n);
        std::swap(dirs[i], dirs[j]);
    }

    for (const auto &d : dirs) {
        int nr = r + d.dr;
        int nc = c + d.dc;

        if (nr <= 0 || nr >= m_gridRows - 1 || nc <= 0 || nc >= m_gridCols - 1)
            continue;

        if (m_grid[nr][nc] == 1) {
            int wallR = r + d.dr / 2;
            int wallC = c + d.dc / 2;
            m_grid[wallR][wallC] = 0;

            carveFrom(nr, nc);
        }
    }
}

// BFS to find farthest reachable passage from `from`
MazeGenerator::Cell MazeGenerator::pickFarthestCell(const Cell &from) const
{
    std::queue<Cell> q;
    std::vector<std::vector<int>> dist(
        m_gridRows, std::vector<int>(m_gridCols, -1));

    auto inside = [&](int r, int c) {
        return r >= 0 && r < m_gridRows && c >= 0 && c < m_gridCols;
    };

    q.push(from);
    dist[from.r][from.c] = 0;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    Cell far = from;
    int best = 0;

    while (!q.empty()) {
        Cell cur = q.front();
        q.pop();
        int curDist = dist[cur.r][cur.c];

        if (curDist > best) {
            best = curDist;
            far = cur;
        }

        for (int k = 0; k < 4; ++k) {
            int nr = cur.r + dr[k];
            int nc = cur.c + dc[k];
            if (!inside(nr, nc))
                continue;
            if (m_grid[nr][nc] == 1)
                continue;
            if (dist[nr][nc] != -1)
                continue;

            dist[nr][nc] = curDist + 1;
            q.push({nr, nc});
        }
    }

    return far;
}

// BFS shortest path from start to goal on passages
std::vector<MazeGenerator::Cell>
MazeGenerator::shortestPath(const Cell &start, const Cell &goal) const
{
    std::queue<Cell> q;
    std::vector<std::vector<bool>> visited(
        m_gridRows, std::vector<bool>(m_gridCols, false));
    std::vector<std::vector<Cell>> parent(
        m_gridRows, std::vector<Cell>(m_gridCols, {-1, -1}));

    auto inside = [&](int r, int c) {
        return r >= 0 && r < m_gridRows && c >= 0 && c < m_gridCols;
    };

    q.push(start);
    visited[start.r][start.c] = true;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    bool found = false;
    while (!q.empty() && !found) {
        Cell cur = q.front();
        q.pop();

        if (cur.r == goal.r && cur.c == goal.c) {
            found = true;
            break;
        }

        for (int k = 0; k < 4; ++k) {
            int nr = cur.r + dr[k];
            int nc = cur.c + dc[k];
            if (!inside(nr, nc))
                continue;
            if (m_grid[nr][nc] == 1)
                continue;
            if (visited[nr][nc])
                continue;

            visited[nr][nc] = true;
            parent[nr][nc] = cur;
            q.push({nr, nc});
        }
    }

    std::vector<Cell> path;
    if (!found)
        return path;

    Cell cur = goal;
    while (!(cur.r == start.r && cur.c == start.c)) {
        path.push_back(cur);
        Cell p = parent[cur.r][cur.c];
        cur = p;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}
