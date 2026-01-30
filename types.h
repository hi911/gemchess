#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstdint>
#include <cassert>
#include "misc.h"

namespace Gemchess {

    using Key = uint64_t;
    using Bitboard = uint64_t;

    constexpr int MAX_MOVES = 256;
    constexpr int MAX_PLY   = 246;

    enum Color { WHITE, BLACK, COLOR_NB = 2 };
    enum PieceType { NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES = 0, PIECE_TYPE_NB = 8 };
    enum Piece {
        NO_PIECE,
        W_PAWN = PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
        PIECE_NB = 16
    };

    enum Square : int {
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
        SQ_NONE,
        SQUARE_NB = 64
    };

    enum Direction : int {
        NORTH =  8, EAST =  1, SOUTH = -8, WEST = -1,
        NORTH_EAST = 9, SOUTH_EAST = -7, SOUTH_WEST = -9, NORTH_WEST = 7
    };

    enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };
    enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

    enum CastlingRights {
        NO_CASTLING,
        WHITE_OO = 1, WHITE_OOO = 2,
        BLACK_OO = 4, BLACK_OOO = 8,
        KING_SIDE = WHITE_OO | BLACK_OO,
        QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
        WHITE_CASTLING = WHITE_OO | WHITE_OOO,
        BLACK_CASTLING = BLACK_OO | BLACK_OOO,
        ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING
    };
    
    inline CastlingRights operator|(CastlingRights a, CastlingRights b) { return CastlingRights(int(a) | int(b)); }
    inline CastlingRights operator&(CastlingRights a, CastlingRights b) { return CastlingRights(int(a) & int(b)); }
    inline CastlingRights& operator|=(CastlingRights& a, CastlingRights b) { return a = a | b; }
    inline CastlingRights& operator&=(CastlingRights& a, CastlingRights b) { return a = a & b; }
    inline CastlingRights operator~(CastlingRights a) { return CastlingRights(~int(a) & 15); }

    enum MoveType {
        NORMAL = 0,
        PROMOTION = 1 << 14,
        EN_PASSANT = 2 << 14,
        CASTLING = 3 << 14
    };

    // --- OPERATORS ---
    inline File& operator++(File& f) { return f = File(f + 1); }
    inline File& operator--(File& f) { return f = File(f - 1); }
    inline Rank& operator++(Rank& r) { return r = Rank(r + 1); }
    inline Rank& operator--(Rank& r) { return r = Rank(r - 1); }
    inline Square& operator++(Square& s) { return s = Square(s + 1); }
    inline Square& operator--(Square& s) { return s = Square(s - 1); }
    constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

    // --- HELPER FUNCTIONS ---
    constexpr Square make_square(File f, Rank r) { return Square((r << 3) + f); }
    constexpr File file_of(Square s) { return File(s & 7); }
    constexpr Rank rank_of(Square s) { return Rank(s >> 3); }
    constexpr bool is_ok(Square s) { return s >= SQ_A1 && s <= SQ_H8; }

    constexpr PieceType type_of(Piece pc) { return PieceType(pc & 7); }
    constexpr Color color_of(Piece pc) { return Color(pc >> 3); }

    // --- MOVE CLASS ---
    class Move {
    public:
        Move() = default;
        constexpr Move(uint16_t d) : data(d) {}
        constexpr Move(Square from, Square to) : data((from << 6) + to) {}

        template<MoveType T>
        static Move make(Square from, Square to, PieceType pt = KNIGHT) {
            return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
        }

        constexpr Square from_sq() const { return Square((data >> 6) & 0x3F); }
        constexpr Square to_sq() const { return Square(data & 0x3F); }
        constexpr MoveType type() const { return MoveType(data & (3 << 14)); }
        constexpr PieceType promotion_type() const { return PieceType(((data >> 12) & 3) + KNIGHT); }
        constexpr uint16_t raw() const { return data; }
        
        static constexpr Move none() { return Move(0); }
        static constexpr Move null() { return Move(65); }
        
        constexpr bool operator==(const Move& m) const { return data == m.data; }
        constexpr bool operator!=(const Move& m) const { return data != m.data; }

    private:
        uint16_t data;
    };
}
#endif