#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <cstdint>
#include <iostream>
#include <chrono>

namespace Gemchess {

    // A fast Pseudo-Random Number Generator (PRNG)
    // Used to generate the "Magic" numbers for bitboards.
    class PRNG {
        uint64_t s;
        uint64_t rand64() {
            s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
            return s * 2685821657736338717LL;
        }

    public:
        PRNG(uint64_t seed) : s(seed) {}
        
        // Return a random number with few bits set (sparse)
        template<typename T> T sparse_rand() {
            return T(rand64() & rand64() & rand64());
        }
    };

    // Current time in milliseconds
    inline uint64_t now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>
               (std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

#endif