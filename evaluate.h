#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include "position.h"

namespace Gemchess {
    
    // Score constants
    constexpr int INF = 30000;
    constexpr int MATE = 29000;

    // Returns score relative to the side to move (positive = good for them)
    int evaluate(const Position& pos);
}
#endif