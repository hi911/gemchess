#ifndef ZOBRIST_H_INCLUDED
#define ZOBRIST_H_INCLUDED

#include "types.h"

namespace Gemchess {
    namespace Zobrist {
        extern Key psq[PIECE_NB][SQUARE_NB];
        extern Key enpassant[FILE_NB];
        extern Key castling[16];
        extern Key side;

        void init();
    }
}
#endif