#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "position.h"
#include <vector>
#include <algorithm>

namespace Gemchess {

    struct MoveList {
        Move moves[MAX_MOVES];
        int scores[MAX_MOVES]; 
        int count = 0;

        void add(Move m) {
            moves[count] = m;
            scores[count] = 0; 
            count++;
        }
        
        Move* begin() { return moves; }
        Move* end() { return moves + count; }
        const Move* begin() const { return moves; }
        const Move* end() const { return moves + count; }
        size_t size() const { return count; }

        void sort() {
            for (int i = 0; i < count - 1; ++i) {
                int max_idx = i;
                for (int j = i + 1; j < count; ++j) {
                    if (scores[j] > scores[max_idx]) max_idx = j;
                }
                std::swap(moves[i], moves[max_idx]);
                std::swap(scores[i], scores[max_idx]);
            }
        }
    };

    MoveList generate_moves(const Position& pos);
}
#endif