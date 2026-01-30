#include "bitboard.h"
#include "misc.h"
#include <cstring>
#include <vector> // Added for vector

namespace Gemchess {

    Magic Magics[SQUARE_NB][2];
    Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
    Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

    // Buffers for attack tables
    Bitboard RookTable[0x19000];
    Bitboard BishopTable[0x1480];

    // Forward declarations
    void init_magics(PieceType pt, Bitboard table[], Magic magics[][2]);

    // Directional Shifts
    Bitboard shift_n(Bitboard b) { return  b << 8; }
    Bitboard shift_s(Bitboard b) { return  b >> 8; }
    Bitboard shift_e(Bitboard b) { return (b & ~FileHBB) << 1; }
    Bitboard shift_w(Bitboard b) { return (b & ~FileABB) >> 1; }
    
    // Simple sliding generation (slow, used only for init)
    Bitboard sliding_attack_slow(PieceType pt, Square sq, Bitboard occupied) {
        Bitboard attacks = 0;
        int dirs[4] = {0};
        if (pt == ROOK) { dirs[0]=8; dirs[1]=-8; dirs[2]=1; dirs[3]=-1; }
        else { dirs[0]=9; dirs[1]=-7; dirs[2]=-9; dirs[3]=7; }

        for (int i=0; i<4; ++i) {
            int d = dirs[i];
            for (int s = sq + d; is_ok(Square(s)) && distance<File>(Square(s-d), Square(s)) <= 1; s += d) {
                attacks |= square_bb(Square(s));
                if (occupied & square_bb(Square(s))) break;
            }
        }
        return attacks;
    }

    void Bitboards::init() {
        // 1. Init Pawn Attacks
        for (int s = 0; s < SQUARE_NB; ++s) {
            Bitboard b = square_bb(Square(s));
            PawnAttacks[WHITE][s] = shift_n(shift_w(b)) | shift_n(shift_e(b));
            PawnAttacks[BLACK][s] = shift_s(shift_w(b)) | shift_s(shift_e(b));
        }

        // 2. Init Knight and King Attacks (FIXED: Added this section)
        for (int s = 0; s < SQUARE_NB; ++s) {
            // Knight Jumps
            int n_steps[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
            for (int step : n_steps) {
                int to = s + step;
                if (is_ok(Square(to)) && distance<File>(Square(s), Square(to)) <= 2)
                    PseudoAttacks[KNIGHT][s] |= square_bb(Square(to));
            }

            // King Steps
            int k_steps[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
            for (int step : k_steps) {
                int to = s + step;
                if (is_ok(Square(to)) && distance<File>(Square(s), Square(to)) <= 1)
                    PseudoAttacks[KING][s] |= square_bb(Square(to));
            }
        }

        // 3. Init Magics for Rooks and Bishops
        init_magics(ROOK, RookTable, Magics);
        init_magics(BISHOP, BishopTable, Magics);
    }

    Bitboard Bitboards::attacks_bb(PieceType pt, Square s, Bitboard occupied) {
        if (pt == PAWN) return PawnAttacks[WHITE][s]; // Dummy color
        
        switch (pt) {
            case ROOK:   return Magics[s][0].attacks[Magics[s][0].index(occupied)];
            case BISHOP: return Magics[s][1].attacks[Magics[s][1].index(occupied)];
            case QUEEN:  return attacks_bb(ROOK, s, occupied) | attacks_bb(BISHOP, s, occupied);
            default:     return PseudoAttacks[pt][s]; // FIXED: Return table for King/Knight
        }
    }

    std::string Bitboards::pretty(Bitboard b) {
        std::string s = "+---+---+---+---+---+---+---+---+\n";
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            for (File f = FILE_A; f <= FILE_H; ++f)
                s += (b & make_square(f, r)) ? "| X " : "|   ";
            s += "|\n+---+---+---+---+---+---+---+---+\n";
        }
        return s;
    }

    // --- MAGIC INITIALIZATION LOGIC ---
    int popcount(Bitboard b) { return __builtin_popcountll(b); }
    
    void init_magics(PieceType pt, Bitboard table[], Magic magics[][2]) {
        int seeds[][RANK_NB] = {{8977, 44560, 54343, 38998, 5731, 95205, 104912, 17020},
                                {728, 10316, 55013, 32803, 12281, 15100, 16645, 255}};
        int size = 0;
        
        for (Square s = SQ_A1; s <= SQ_H8; ++s) {
            Bitboard edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));
            Magic& m = magics[s][pt == ROOK ? 0 : 1];
            m.mask = sliding_attack_slow(pt, s, 0) & ~edges;
            m.shift = 64 - popcount(m.mask);
            m.attacks = (s == SQ_A1) ? table : magics[s - 1][pt == ROOK ? 0 : 1].attacks + size;

            Bitboard b = 0;
            int permutations = (1 << popcount(m.mask));
            
            std::vector<Bitboard> reference(permutations);
            std::vector<Bitboard> occupancies(permutations);
            
            for(int i=0; i<permutations; ++i) {
                occupancies[i] = b;
                reference[i] = sliding_attack_slow(pt, s, b);
                b = (b - m.mask) & m.mask;
            }
            size = 0; 

            PRNG rng(seeds[0][rank_of(s)]);
            bool found = false;
            while(!found) {
                m.magic = rng.sparse_rand<Bitboard>();
                if (popcount((m.magic * m.mask) >> 56) < 6) continue;
                
                std::memset(m.attacks, 0, permutations * sizeof(Bitboard));
                found = true;
                for(int i=0; i<permutations; ++i) {
                    unsigned idx = m.index(occupancies[i]);
                    if (m.attacks[idx] != 0 && m.attacks[idx] != reference[i]) {
                        found = false; break;
                    }
                    m.attacks[idx] = reference[i];
                }
            }
            size = permutations;
        }
    }
}