#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED

#include <string>
#include <vector>
#include <cstdlib> // ADDED for std::abs
#include "types.h"

namespace Gemchess {

    namespace Bitboards {
        void init();
        std::string pretty(Bitboard b);
        
        Bitboard attacks_bb(PieceType pt, Square s, Bitboard occupied);
    }

    constexpr Bitboard FileABB = 0x0101010101010101ULL;
    constexpr Bitboard FileHBB = FileABB << 7;
    constexpr Bitboard Rank1BB = 0xFF;
    constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

    // --- HELPER FUNCTIONS (New!) ---
    constexpr Bitboard rank_bb(Rank r) { return Rank1BB << (8 * r); }
    constexpr Bitboard rank_bb(Square s) { return rank_bb(rank_of(s)); }
    constexpr Bitboard file_bb(File f) { return FileABB << f; }
    constexpr Bitboard file_bb(Square s) { return file_bb(file_of(s)); }

    template<typename T> inline int distance(Square x, Square y);
    template<> inline int distance<File>(Square x, Square y) { return std::abs(file_of(x) - file_of(y)); }
    template<> inline int distance<Rank>(Square x, Square y) { return std::abs(rank_of(x) - rank_of(y)); }

    // --- MAGIC BITBOARDS STRUCT ---
    struct Magic {
        Bitboard mask;
        Bitboard* attacks;
        Bitboard magic;
        unsigned shift;

        unsigned index(Bitboard occupied) const {
            return unsigned(((occupied & mask) * magic) >> shift);
        }
    };
    
    extern Magic Magics[SQUARE_NB][2];

    // --- INLINE BITBOARD OPS ---
    constexpr Bitboard square_bb(Square s) { return (1ULL << s); }
    
    constexpr Bitboard  operator&(Bitboard b, Square s) { return b & square_bb(s); }
    constexpr Bitboard  operator|(Bitboard b, Square s) { return b | square_bb(s); }
    constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
    
    inline Square lsb(Bitboard b) {
        return Square(__builtin_ctzll(b));
    }
    
    inline Square pop_lsb(Bitboard& b) {
        Square s = lsb(b);
        b &= b - 1;
        return s;
    }
}
#endif