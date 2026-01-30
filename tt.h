#ifndef TT_H_INCLUDED
#define TT_H_INCLUDED

#include "types.h"

namespace Gemchess {

    enum TTFlag {
        TT_EXACT,
        TT_ALPHA, 
        TT_BETA   
    };

    struct TTEntry {
        Key key;
        Move move;
        int16_t score;
        int16_t depth;
        uint8_t flag;
    };

    class TranspositionTable {
    public:
        void resize(size_t mbSize);
        void clear();
        void save(Key key, int score, int depth, TTFlag flag, Move move);
        bool probe(Key key, int& score, int depth, int alpha, int beta, Move& move);
        Move probe_move(Key key);

    private:
        TTEntry* table = nullptr;
        size_t count = 0;
    };

    extern TranspositionTable TT;
}
#endif