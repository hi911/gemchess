#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <string>
#include <iostream>
#include "types.h"
#include "bitboard.h"

namespace Gemchess {

    const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    struct StateInfo {
        CastlingRights castlingRights;
        Square epSquare;
        Piece capturedPiece;
        Key key; // Zobrist Hash of this state
        StateInfo* previous = nullptr; 
    };

    class Position {
    public:
        Position() = default;
        void set(const std::string& fen);

        Bitboard pieces(PieceType pt) const { return byType[pt]; }
        Bitboard pieces(Color c) const { return byColor[c]; }
        Bitboard pieces(Color c, PieceType pt) const { return byColor[c] & byType[pt]; }
        Bitboard pieces() const { return byType[ALL_PIECES]; } 
        Piece piece_on(Square s) const;

        Color side_to_move() const { return sideToMove; }
        CastlingRights castling_rights() const { return castlingRights; }
        Square ep_square() const { return epSquare; }
        Square king_square(Color c) const;
        Key key() const { return st->key; } // Accessor for the hash

        Bitboard attackers_to(Square s, Color c) const;

        void do_move(Move m, StateInfo& newSt);
        void undo_move(Move m);
        void do_null_move(StateInfo& newSt);
        void undo_null_move();

        friend std::ostream& operator<<(std::ostream& os, const Position& pos);

    private:
        Bitboard byType[PIECE_TYPE_NB];
        Bitboard byColor[COLOR_NB];
        Piece board[SQUARE_NB]; 
        Color sideToMove = WHITE;
        CastlingRights castlingRights = NO_CASTLING;
        Square epSquare = SQ_NONE;
        
        StateInfo* st = nullptr;
        // Temporary start state to avoid crashes if no moves made yet
        StateInfo startState; 

        void clear();
        void put_piece(Piece p, Square s);
        void remove_piece(Square s);
        void move_piece(Square from, Square to);
        
        // Helper to calculate full hash from scratch
        Key compute_key() const;
    };
}
#endif