#include "tt.h"
#include "evaluate.h" // For MATE constants
#include <cstring>
#include <iostream>

namespace Gemchess {

    TranspositionTable TT;

    void TranspositionTable::resize(size_t mbSize) {
        if (table) delete[] table;
        
        // Calculate number of entries that fit in mbSize
        size_t size = (mbSize * 1024 * 1024) / sizeof(TTEntry);
        count = size;
        table = new TTEntry[count];
        clear();
    }

    void TranspositionTable::clear() {
        std::memset(table, 0, count * sizeof(TTEntry));
    }

    // Adjust mate scores so they are relative to the current ply, not the root
    int score_to_tt(int score, int ply) {
        if (score > MATE - 1000) return score + ply;
        if (score < -MATE + 1000) return score - ply;
        return score;
    }

    int score_from_tt(int score, int ply) {
        if (score > MATE - 1000) return score - ply;
        if (score < -MATE + 1000) return score + ply;
        return score;
    }

    void TranspositionTable::save(Key key, int score, int depth, TTFlag flag, Move move) {
        if (count == 0) return; // No hash allocated

        TTEntry& entry = table[key % count];

        // Replacement scheme: Deepest search or different position
        // Simple overwrite if depth is higher or same
        if (entry.key != key || depth >= entry.depth) {
            entry.key = key;
            entry.depth = (int16_t)depth;
            entry.flag = (uint8_t)flag;
            entry.score = (int16_t)score_to_tt(score, 0); // Simplified ply handling for V1
            if (move.raw() != 0) entry.move = move; // Don't overwrite best move with null
        }
    }

    bool TranspositionTable::probe(Key key, int& score, int depth, int alpha, int beta, Move& move) {
        if (count == 0) return false;

        TTEntry& entry = table[key % count];
        if (entry.key == key) {
            move = entry.move; // Always return the best move for ordering
            
            if (entry.depth >= depth) {
                int ttScore = score_from_tt(entry.score, 0);
                
                if (entry.flag == TT_EXACT) {
                    score = ttScore;
                    return true;
                }
                if (entry.flag == TT_ALPHA && ttScore <= alpha) {
                    score = alpha;
                    return true;
                }
                if (entry.flag == TT_BETA && ttScore >= beta) {
                    score = beta;
                    return true;
                }
            }
        }
        return false;
    }

    Move TranspositionTable::probe_move(Key key) {
        if (count == 0) return Move::none();
        TTEntry& entry = table[key % count];
        return (entry.key == key) ? entry.move : Move::none();
    }
}