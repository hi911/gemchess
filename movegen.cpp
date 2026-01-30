#include "movegen.h"
#include "bitboard.h" 

namespace Gemchess {

    using namespace Bitboards; 

    MoveList generate_moves(const Position& pos) {
        MoveList list;
        Color us = pos.side_to_move();
        Color them = ~us;

        Bitboard ourPieces = pos.pieces(us);
        Bitboard theirPieces = pos.pieces(them);
        Bitboard allPieces = ourPieces | theirPieces;

        // --- PAWN MOVES ---
        Bitboard pawns = pos.pieces(us, PAWN);
        Direction up = (us == WHITE) ? NORTH : SOUTH;
        Direction upRight = (us == WHITE) ? NORTH_EAST : SOUTH_EAST;
        Direction upLeft = (us == WHITE) ? NORTH_WEST : SOUTH_WEST;
        Rank promRank = (us == WHITE) ? RANK_8 : RANK_1;
        
        auto add_pawn_move = [&](Square from, Square to) {
            if (rank_of(to) == promRank) {
                list.add(Move::make<PROMOTION>(from, to, QUEEN));
                list.add(Move::make<PROMOTION>(from, to, ROOK));
                list.add(Move::make<PROMOTION>(from, to, BISHOP));
                list.add(Move::make<PROMOTION>(from, to, KNIGHT));
            } else {
                list.add(Move(from, to));
            }
        };

        // 1. Single Push
        Bitboard singlePush = (us == WHITE ? (pawns << 8) : (pawns >> 8)) & ~allPieces;
        Bitboard singlePushCopy = singlePush;
        while (singlePushCopy) {
            Square to = pop_lsb(singlePushCopy);
            add_pawn_move(Square(to - up), to);
        }

        // 2. Double Push
        Bitboard doublePush;
        if (us == WHITE) 
            doublePush = ((singlePush & rank_bb(RANK_3)) << 8) & ~allPieces;
        else 
            doublePush = ((singlePush & rank_bb(RANK_6)) >> 8) & ~allPieces;
        
        while (doublePush) {
            Square to = pop_lsb(doublePush);
            list.add(Move(Square(to - up - up), to));
        }

        // 3. Captures
        Bitboard pawnsCopy = pawns;
        while (pawnsCopy) {
            Square from = pop_lsb(pawnsCopy);
            Square to_l = Square(from + upLeft);
            Square to_r = Square(from + upRight);
            
            if (distance<File>(from, to_l) == 1 && (theirPieces & square_bb(to_l))) 
                add_pawn_move(from, to_l);
                
            if (distance<File>(from, to_r) == 1 && (theirPieces & square_bb(to_r))) 
                add_pawn_move(from, to_r);

            if (pos.ep_square() != SQ_NONE) {
                if (to_l == pos.ep_square() && distance<File>(from, to_l) == 1) 
                    list.add(Move::make<EN_PASSANT>(from, to_l));
                if (to_r == pos.ep_square() && distance<File>(from, to_r) == 1) 
                    list.add(Move::make<EN_PASSANT>(from, to_r));
            }
        }

        // --- PIECE MOVES ---
        Bitboard nonPawns = ourPieces & ~pos.pieces(PAWN);
        while (nonPawns) {
            Square from = pop_lsb(nonPawns);
            PieceType pt = type_of(Piece(type_of(Piece(pos.pieces(KNIGHT) & (1ULL << from) ? W_KNIGHT :
                                          pos.pieces(BISHOP) & (1ULL << from) ? W_BISHOP :
                                          pos.pieces(ROOK) & (1ULL << from) ? W_ROOK :
                                          pos.pieces(QUEEN) & (1ULL << from) ? W_QUEEN :
                                          W_KING)))); 
            
            if (pos.pieces(KNIGHT) & (1ULL<<from)) pt = KNIGHT;
            else if (pos.pieces(BISHOP) & (1ULL<<from)) pt = BISHOP;
            else if (pos.pieces(ROOK) & (1ULL<<from)) pt = ROOK;
            else if (pos.pieces(QUEEN) & (1ULL<<from)) pt = QUEEN;
            else if (pos.pieces(KING) & (1ULL<<from)) pt = KING;

            Bitboard attacks = Bitboards::attacks_bb(pt, from, allPieces);
            Bitboard validMoves = attacks & ~ourPieces;

            while (validMoves) {
                Square to = pop_lsb(validMoves);
                list.add(Move(from, to));
            }
        }

        // --- CASTLING ---
        if (us == WHITE) {
            if (pos.castling_rights() & WHITE_OO) {
                if (!(allPieces & (square_bb(SQ_F1) | square_bb(SQ_G1)))) {
                    if (!pos.attackers_to(SQ_E1, BLACK) && !pos.attackers_to(SQ_F1, BLACK) && !pos.attackers_to(SQ_G1, BLACK))
                        list.add(Move::make<CASTLING>(SQ_E1, SQ_G1));
                }
            }
            if (pos.castling_rights() & WHITE_OOO) {
                if (!(allPieces & (square_bb(SQ_D1) | square_bb(SQ_C1) | square_bb(SQ_B1)))) {
                    if (!pos.attackers_to(SQ_E1, BLACK) && !pos.attackers_to(SQ_D1, BLACK) && !pos.attackers_to(SQ_C1, BLACK))
                        list.add(Move::make<CASTLING>(SQ_E1, SQ_C1));
                }
            }
        } else {
            if (pos.castling_rights() & BLACK_OO) {
                if (!(allPieces & (square_bb(SQ_F8) | square_bb(SQ_G8)))) {
                    if (!pos.attackers_to(SQ_E8, WHITE) && !pos.attackers_to(SQ_F8, WHITE) && !pos.attackers_to(SQ_G8, WHITE))
                        list.add(Move::make<CASTLING>(SQ_E8, SQ_G8));
                }
            }
            if (pos.castling_rights() & BLACK_OOO) {
                if (!(allPieces & (square_bb(SQ_D8) | square_bb(SQ_C8) | square_bb(SQ_B8)))) {
                    if (!pos.attackers_to(SQ_E8, WHITE) && !pos.attackers_to(SQ_D8, WHITE) && !pos.attackers_to(SQ_C8, WHITE))
                        list.add(Move::make<CASTLING>(SQ_E8, SQ_C8));
                }
            }
        }

        return list;
    }
}