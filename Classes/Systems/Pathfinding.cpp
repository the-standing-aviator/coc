#include "Pathfinding.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <limits>

using namespace cocos2d;

Vec2 Pathfinding::stepTowards(const Vec2& cur, const Vec2& dst, float step, float stopDistance)
{
    Vec2 v = dst - cur;
    float dist = v.length();
    if (dist <= stopDistance) return cur;
    if (dist <= 0.0001f) return cur;

    float maxAdvance = std::max(0.0f, dist - stopDistance);
    float adv = std::min(step, maxAdvance);

    Vec2 dir = v / dist;
    return cur + dir * adv;
}

static inline int idxOf(int r, int c, int cols) { return r * cols + c; }

std::vector<Pathfinding::GridPos> Pathfinding::findPathAStar(int rows, int cols,
    GridPos start, GridPos goal,
    const std::vector<unsigned char>& blocked,
    bool allowDiag,
    int maxIters)
{
    std::vector<GridPos> empty;
    if (rows <= 0 || cols <= 0) return empty;
    if (start.r < 0 || start.r >= rows || start.c < 0 || start.c >= cols) return empty;
    if (goal.r < 0 || goal.r >= rows || goal.c < 0 || goal.c >= cols) return empty;

    const int N = rows * cols;
    if ((int)blocked.size() != N) return empty;

    // start and goal must be passable
    if (blocked[idxOf(start.r, start.c, cols)]) return empty;
    if (blocked[idxOf(goal.r, goal.c, cols)]) return empty;

    auto h = [&](int r, int c) -> float {
        return (float)(std::abs(r - goal.r) + std::abs(c - goal.c));
        };

    std::vector<float> g(N, std::numeric_limits<float>::infinity());
    std::vector<int> parent(N, -1);
    std::vector<unsigned char> closed(N, 0);

    struct Node {
        float f;
        int idx;
        bool operator>(const Node& o) const { return f > o.f; }
    };
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

    int sidx = idxOf(start.r, start.c, cols);
    g[sidx] = 0.0f;
    open.push({ h(start.r, start.c), sidx });

    int iters = 0;
    const int dirs4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    const int dirs8[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };

    while (!open.empty() && iters++ < maxIters)
    {
        Node cur = open.top();
        open.pop();

        int ci = cur.idx;
        if (closed[ci]) continue;
        closed[ci] = 1;

        int cr = ci / cols;
        int cc = ci % cols;

        if (cr == goal.r && cc == goal.c)
        {
            // reconstruct
            std::vector<GridPos> path;
            int p = ci;
            while (p != -1)
            {
                path.push_back({ p / cols, p % cols });
                p = parent[p];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        const int (*dirs)[2] = allowDiag ? dirs8 : dirs4;
        const int dcount = allowDiag ? 8 : 4;

        for (int k = 0; k < dcount; ++k)
        {
            int nr = cr + dirs[k][0];
            int nc = cc + dirs[k][1];
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;

            int ni = idxOf(nr, nc, cols);
            if (closed[ni]) continue;
            if (blocked[ni]) continue;

            // cost: 1 for orthogonal, 1.414 for diagonal
            float stepCost = 1.0f;
            if (allowDiag && k >= 4) stepCost = 1.41421356f;

            float ng = g[ci] + stepCost;
            if (ng < g[ni])
            {
                g[ni] = ng;
                parent[ni] = ci;
                float nf = ng + h(nr, nc);
                open.push({ nf, ni });
            }
        }
    }

    return empty;
}

std::vector<Vec2> Pathfinding::makeDirectPath(const Vec2& start, const Vec2& end, int segments)
{
    std::vector<Vec2> path;
    segments = std::max(1, segments);
    path.reserve((size_t)segments + 1);
    path.push_back(start);
    for (int i = 1; i < segments; ++i)
    {
        float t = (float)i / (float)segments;
        path.push_back(start.lerp(end, t));
    }
    path.push_back(end);
    return path;
}
