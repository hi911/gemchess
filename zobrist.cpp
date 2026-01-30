#include "zobrist.h"
#include "misc.h"

namespace Gemchess {
    namespace Zobrist {
        Key psq[PIECE_NB][SQUARE_NB];
        Key enpassant[FILE_NB];
        Key castling[16];
        Key side;

        void init() {
            PRNG rng(1070372); // Fixed seed for reproducibility
            
            for (int p = 0; p < PIECE_NB; ++p)
                for (int s = 0; s < SQUARE_NB; ++s)
                    psq[p][s] = rng.sparse_rand<Key>();

            for (int f = 0; f < FILE_NB; ++f)
                enpassant[f] = rng.sparse_rand<Key>();

            for (int c = 0; c < 16; ++c)
                castling[c] = rng.sparse_rand<Key>();

            side = rng.sparse_rand<Key>();
        }
    }
}