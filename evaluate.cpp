#include "evaluate.h"

namespace Gemchess {

    // Simplified PeSTO values (Midgame)
    constexpr int PieceValue[] = { 0, 100, 320, 330, 500, 900, 20000 };

    // Tables defined from A1 (bottom-left) to H8 (top-right)
    // Rank 1 .. Rank 8
    
    const int PawnTable[] = {
         0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-20,-20, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    
    // BUT! We access them linearly. 
    // To make it intuitive (visual), we define them Rank 8 -> Rank 1
    // and then map carefully. 
    // Let's use standard "White's Perspective" tables (Rank 8 at top).
    
    // FLIP function needed? No, let's just define them clearly.
    // Index 0 = A1, Index 63 = H8.
    
    const int PawnPST[] = {
        0,   0,   0,   0,   0,   0,   0,   0,   // Rank 1
        5,  10,  10, -20, -20,  10,  10,   5,   // Rank 2
        5,  -5, -10,   0,   0, -10,  -5,   5,   // Rank 3
        0,   0,   0,  20,  20,   0,   0,   0,   // Rank 4
        5,   5,  10,  25,  25,  10,   5,   5,   // Rank 5
       10,  10,  20,  30,  30,  20,  10,  10,   // Rank 6
       50,  50,  50,  50,  50,  50,  50,  50,   // Rank 7
        0,   0,   0,   0,   0,   0,   0,   0    // Rank 8
    };

    const int KnightPST[] = {
       -50, -40, -30, -30, -30, -30, -40, -50,
       -40, -20,   0,   5,   5,   0, -20, -40,
       -30,   5,  10,  15,  15,  10,   5, -30,
       -30,   0,  15,  20,  20,  15,   0, -30,
       -30,   5,  15,  20,  20,  15,   5, -30,
       -30,   0,  10,  15,  15,  10,   0, -30,
       -40, -20,   0,   0,   0,   0, -20, -40,
       -50, -40, -30, -30, -30, -30, -40, -50
    };

    const int BishopPST[] = {
       -20, -10, -10, -10, -10, -10, -10, -20,
       -10,   5,   0,   0,   0,   0,   5, -10,
       -10,  10,  10,  10,  10,  10,  10, -10,
       -10,   0,  10,  10,  10,  10,   0, -10,
       -10,   5,   5,  10,  10,   5,   5, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -20, -10, -10, -10, -10, -10, -10, -20
    };

    const int RookPST[] = {
         0,   0,   0,   5,   5,   0,   0,   0,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
         5,  10,  10,  10,  10,  10,  10,   5,
         0,   0,   0,   0,   0,   0,   0,   0
    };

    const int QueenPST[] = {
       -20, -10, -10,  -5,  -5, -10, -10, -20,
       -10,   0,   5,   0,   0,   0,   0, -10,
       -10,   5,   5,   5,   5,   5,   0, -10,
         0,   0,   5,   5,   5,   5,   0,  -5,
        -5,   0,   5,   5,   5,   5,   0,  -5,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -20, -10, -10,  -5,  -5, -10, -10, -20
    };

    // King safety (Midgame)
    const int KingPST[] = {
        20,  30,  10,   0,   0,  10,  30,  20,
        20,  20,   0,   0,   0,   0,  20,  20,
       -10, -20, -20, -20, -20, -20, -20, -10,
       -20, -30, -30, -40, -40, -30, -30, -20,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30
    };

    // A1=0, A8=56. To mirror for Black:
    // Square 0 (A1) -> Square 56 (A8).
    // Square s -> s ^ 56
    inline int mirror(int sq) { return sq ^ 56; }

    int evaluate(const Position& pos) {
        int mg[2] = {0, 0}; 

        for (int c = 0; c < COLOR_NB; ++c) {
            Color color = Color(c);
            
            // Pawns
            Bitboard b = pos.pieces(color, PAWN);
            while (b) {
                Square s = pop_lsb(b);
                // If White, use 's'. If Black, use 'mirror(s)' to map A8->A1 logic
                mg[c] += PieceValue[PAWN] + PawnPST[c == WHITE ? s : mirror(s)];
            }

            // Knights
            b = pos.pieces(color, KNIGHT);
            while (b) {
                Square s = pop_lsb(b);
                mg[c] += PieceValue[KNIGHT] + KnightPST[c == WHITE ? s : mirror(s)];
            }

            // Bishops
            b = pos.pieces(color, BISHOP);
            while (b) {
                Square s = pop_lsb(b);
                mg[c] += PieceValue[BISHOP] + BishopPST[c == WHITE ? s : mirror(s)];
            }

            // Rooks
            b = pos.pieces(color, ROOK);
            while (b) {
                Square s = pop_lsb(b);
                mg[c] += PieceValue[ROOK] + RookPST[c == WHITE ? s : mirror(s)];
            }

            // Queens
            b = pos.pieces(color, QUEEN);
            while (b) {
                Square s = pop_lsb(b);
                mg[c] += PieceValue[QUEEN] + QueenPST[c == WHITE ? s : mirror(s)];
            }

            // King
            b = pos.pieces(color, KING);
            while (b) {
                Square s = pop_lsb(b);
                mg[c] += PieceValue[KING] + KingPST[c == WHITE ? s : mirror(s)];
            }
        }

        int score = mg[pos.side_to_move()] - mg[~pos.side_to_move()];
        return score;
    }
}