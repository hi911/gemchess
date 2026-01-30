#include <sstream>
#include <cstring>
#include "position.h"
#include "zobrist.h"

namespace Gemchess {

    void Position::clear() {
        std::memset(byType, 0, sizeof(byType));
        std::memset(byColor, 0, sizeof(byColor));
        std::memset(board, 0, sizeof(board));
        sideToMove = WHITE;
        castlingRights = NO_CASTLING;
        epSquare = SQ_NONE;
        st = &startState; // Point to internal safe state
    }

    void Position::put_piece(Piece p, Square s) {
        Bitboard b = square_bb(s);
        byType[ALL_PIECES] |= b;
        byType[type_of(p)] |= b;   
        byColor[color_of(p)] |= b;
        board[s] = p;
    }

    void Position::remove_piece(Square s) {
        Piece p = board[s];
        Bitboard b = square_bb(s);
        byType[ALL_PIECES] &= ~b;
        byType[type_of(p)] &= ~b;
        byColor[color_of(p)] &= ~b;
        board[s] = NO_PIECE;
    }

    void Position::move_piece(Square from, Square to) {
        Piece p = board[from];
        Bitboard fromB = square_bb(from);
        Bitboard toB = square_bb(to);
        Bitboard combo = fromB | toB;

        byType[ALL_PIECES] ^= combo;
        byType[type_of(p)] ^= combo;
        byColor[color_of(p)] ^= combo;
        board[from] = NO_PIECE;
        board[to] = p;
    }

    Key Position::compute_key() const {
        Key k = 0;
        for (int s = 0; s < SQUARE_NB; ++s) {
            Piece p = board[s];
            if (p != NO_PIECE) k ^= Zobrist::psq[p][s];
        }
        if (sideToMove == BLACK) k ^= Zobrist::side;
        if (epSquare != SQ_NONE) k ^= Zobrist::enpassant[file_of(epSquare)];
        k ^= Zobrist::castling[castlingRights];
        return k;
    }

    Piece Position::piece_on(Square s) const { return board[s]; }
    Square Position::king_square(Color c) const { return lsb(pieces(c, KING)); }

    void Position::do_move(Move m, StateInfo& newSt) {
        newSt.castlingRights = castlingRights;
        newSt.epSquare = epSquare;
        newSt.capturedPiece = NO_PIECE;
        newSt.previous = st;
        newSt.key = st->key; // Start with current key
        st = &newSt;         

        Square from = m.from_sq();
        Square to = m.to_sq();
        Color us = sideToMove;
        Color them = ~us;
        Piece movingPiece = board[from];
        MoveType type = m.type();

        // Key Update: Remove moving piece from 'from'
        st->key ^= Zobrist::psq[movingPiece][from];

        if (board[to] != NO_PIECE) {
            newSt.capturedPiece = board[to];
            // Key Update: Remove captured piece
            st->key ^= Zobrist::psq[board[to]][to];
            remove_piece(to);
        } else if (type == EN_PASSANT) {
            Square capSq = make_square(file_of(to), rank_of(from)); 
            newSt.capturedPiece = board[capSq];
            // Key Update: Remove en passant pawn
            st->key ^= Zobrist::psq[board[capSq]][capSq];
            remove_piece(capSq);
        }

        move_piece(from, to);
        // Key Update: Add moving piece to 'to'
        st->key ^= Zobrist::psq[movingPiece][to];

        if (type == CASTLING) {
            Square rFrom, rTo;
            if (to > from) { rFrom = make_square(FILE_H, rank_of(from)); rTo = make_square(FILE_F, rank_of(from)); } 
            else { rFrom = make_square(FILE_A, rank_of(from)); rTo = make_square(FILE_D, rank_of(from)); }
            
            // Key Update: Move rook
            Piece rook = board[rFrom];
            st->key ^= Zobrist::psq[rook][rFrom];
            st->key ^= Zobrist::psq[rook][rTo];
            move_piece(rFrom, rTo);
        } 
        else if (type == PROMOTION) {
            Piece promoted = Piece(m.promotion_type() + (us == WHITE ? 0 : 8));
            // Key Update: Remove pawn at 'to', add promoted piece at 'to'
            st->key ^= Zobrist::psq[movingPiece][to];
            st->key ^= Zobrist::psq[promoted][to];
            
            remove_piece(to); 
            put_piece(promoted, to); 
        }

        // Key Update: Update Castling Rights (Remove old, add new later)
        st->key ^= Zobrist::castling[castlingRights];
        
        // Key Update: Update EP Square (Remove old)
        if (epSquare != SQ_NONE) st->key ^= Zobrist::enpassant[file_of(epSquare)];

        epSquare = SQ_NONE; 
        if (type_of(movingPiece) == PAWN) {
            if (distance<Rank>(from, to) == 2) {
                epSquare = make_square(file_of(from), Rank((rank_of(from) + rank_of(to)) / 2));
                // Key Update: Add new EP Square
                st->key ^= Zobrist::enpassant[file_of(epSquare)];
            }
        }

        if (castlingRights != NO_CASTLING) {
             if (type_of(movingPiece) == KING) castlingRights &= (us == WHITE ? ~WHITE_CASTLING : ~BLACK_CASTLING);
             if (from == SQ_A1 || to == SQ_A1) castlingRights &= ~WHITE_OOO;
             if (from == SQ_H1 || to == SQ_H1) castlingRights &= ~WHITE_OO;
             if (from == SQ_A8 || to == SQ_A8) castlingRights &= ~BLACK_OOO;
             if (from == SQ_H8 || to == SQ_H8) castlingRights &= ~BLACK_OO;
        }
        
        // Key Update: Add new Castling Rights
        st->key ^= Zobrist::castling[castlingRights];
        
        // Key Update: Swap side to move
        st->key ^= Zobrist::side;

        sideToMove = them;
    }

    void Position::undo_move(Move m) {
        Color us = ~sideToMove;
        Square from = m.from_sq();
        Square to = m.to_sq();
        MoveType type = m.type();

        sideToMove = us;
        castlingRights = st->castlingRights;
        epSquare = st->epSquare;
        
        if (type == PROMOTION) {
            remove_piece(to);
            put_piece(Piece(PAWN + (us == WHITE ? 0 : 8)), to);
            move_piece(to, from);
        } else {
            move_piece(to, from);
        }

        if (st->capturedPiece != NO_PIECE) {
            if (type == EN_PASSANT) {
                Square capSq = make_square(file_of(to), rank_of(from));
                put_piece(st->capturedPiece, capSq);
            } else {
                put_piece(st->capturedPiece, to);
            }
        }
        else if (type == CASTLING) {
            Square rFrom, rTo;
            if (to > from) { rFrom = make_square(FILE_H, rank_of(from)); rTo = make_square(FILE_F, rank_of(from)); } 
            else { rFrom = make_square(FILE_A, rank_of(from)); rTo = make_square(FILE_D, rank_of(from)); }
            move_piece(rTo, rFrom);
        }
        st = st->previous;
    }

    void Position::do_null_move(StateInfo& newSt) {
        newSt.castlingRights = castlingRights;
        newSt.epSquare = epSquare;
        newSt.capturedPiece = NO_PIECE;
        newSt.previous = st;
        newSt.key = st->key; // Copy key
        st = &newSt;

        // Hash Updates for Null Move
        if (epSquare != SQ_NONE) st->key ^= Zobrist::enpassant[file_of(epSquare)];
        epSquare = SQ_NONE;
        
        st->key ^= Zobrist::side;
        sideToMove = ~sideToMove;
    }

    void Position::undo_null_move() {
        sideToMove = ~sideToMove;
        castlingRights = st->castlingRights;
        epSquare = st->epSquare;
        st = st->previous;
    }

    void Position::set(const std::string& fen) {
        clear();
        std::istringstream ss(fen);
        std::string boardToken, sideToken, castleToken, epToken;
        ss >> boardToken >> sideToken >> castleToken >> epToken;

        size_t fenIdx = 0;
        Rank rank = RANK_8;
        File file = FILE_A;
        while (fenIdx < boardToken.length() && rank >= RANK_1) {
            char token = boardToken[fenIdx++];
            if (isdigit(token)) file = File(file + (token - '0')); 
            else if (token == '/') { rank = Rank(rank - 1); file = FILE_A; } 
            else {
                Color c = isupper(token) ? WHITE : BLACK;
                PieceType pt = NO_PIECE_TYPE;
                char lower = tolower(token);
                if (lower == 'p') pt = PAWN; else if (lower == 'n') pt = KNIGHT;
                else if (lower == 'b') pt = BISHOP; else if (lower == 'r') pt = ROOK;
                else if (lower == 'q') pt = QUEEN; else if (lower == 'k') pt = KING;
                if (pt != NO_PIECE_TYPE) {
                    put_piece(c == WHITE ? Piece(pt) : Piece(pt + 8), make_square(file, rank));
                    file = File(file + 1);
                }
            }
        }
        sideToMove = (sideToken == "w" ? WHITE : BLACK);
        
        if (castleToken != "-") {
            for (char c : castleToken) {
                if (c == 'K') castlingRights |= WHITE_OO; 
                if (c == 'Q') castlingRights |= WHITE_OOO;
                if (c == 'k') castlingRights |= BLACK_OO; 
                if (c == 'q') castlingRights |= BLACK_OOO;
            }
        }
        if (epToken != "-") {
            File f = File(epToken[0] - 'a'); Rank r = Rank(epToken[1] - '1');
            epSquare = make_square(f, r);
        }
        
        // Calculate initial hash
        st->key = compute_key();
    }

    Bitboard Position::attackers_to(Square s, Color c) const {
        Bitboard occupied = pieces();
        Bitboard attackers = (Bitboards::attacks_bb(KNIGHT, s, 0) & pieces(c, KNIGHT))
                           | (Bitboards::attacks_bb(KING, s, 0)   & pieces(c, KING))
                           | (Bitboards::attacks_bb(ROOK, s, occupied)   & (pieces(c, ROOK) | pieces(c, QUEEN)))
                           | (Bitboards::attacks_bb(BISHOP, s, occupied) & (pieces(c, BISHOP) | pieces(c, QUEEN)));
        
        Bitboard pawn_lookup = 0;
        if (c == WHITE) {
            if (rank_of(s) > RANK_2) {
                if (file_of(s) > FILE_A) pawn_lookup |= square_bb(Square(s - 9));
                if (file_of(s) < FILE_H) pawn_lookup |= square_bb(Square(s - 7));
            }
        } else {
             if (rank_of(s) < RANK_7) {
                if (file_of(s) > FILE_A) pawn_lookup |= square_bb(Square(s + 7));
                if (file_of(s) < FILE_H) pawn_lookup |= square_bb(Square(s + 9));
            }
        }
        attackers |= (pawn_lookup & pieces(c, PAWN));
        return attackers;
    }

    std::ostream& operator<<(std::ostream& os, const Position& pos) {
        os << "\n +---+---+---+---+---+---+---+---+\n";
        for (Rank r = RANK_8; r >= RANK_1; --r) {
            for (File f = FILE_A; f <= FILE_H; ++f) {
                Square s = make_square(f, r);
                char c = ' ';
                Piece p = pos.board[s];
                if (p != NO_PIECE) {
                    char symbol[] = " PNBRQKpnbrqk";
                    int idx = (type_of(p) + (color_of(p) == BLACK ? 6 : 0));
                    c = symbol[idx];
                }
                os << " | " << c;
            }
            os << " | " << (r + 1) << "\n +---+---+---+---+---+---+---+---+\n";
        }
        os << "   a   b   c   d   e   f   g   h\n\n";
        os << "Hash: " << std::hex << pos.st->key << std::dec << "\n";
        return os;
    }
}