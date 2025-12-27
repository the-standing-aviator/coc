#pragma once
#include "cocos2d.h"
#include <vector>

namespace Pathfinding {

    struct GridPos {
        int r = 0;
        int c = 0;
    };

    // Simple step towards dst in world space.
    // stopDistance: if within this distance to dst, don't move closer.
    cocos2d::Vec2 stepTowards(const cocos2d::Vec2& cur,
        const cocos2d::Vec2& dst,
        float step,
        float stopDistance);

    // A* on a grid [rows x cols].
    // blocked: size rows*cols, 1=blocked, 0=passable
    // allowDiag: if true, 8-dir neighbors
    std::vector<GridPos> findPathAStar(int rows, int cols,
        GridPos start, GridPos goal,
        const std::vector<unsigned char>& blocked,
        bool allowDiag,
        int maxIters);

    // Optional: build a direct path for debugging
    std::vector<cocos2d::Vec2> makeDirectPath(const cocos2d::Vec2& start,
        const cocos2d::Vec2& end,
        int segments);
}
