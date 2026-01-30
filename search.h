#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "position.h"
#include <atomic>

namespace Gemchess {
    
    struct SearchLimits {
        int depth = -1;
        int movetime = -1;
        int time[2] = {0, 0}; 
        int inc[2] = {0, 0};
        bool infinite = false;
        int threads = 1; 
    };

    extern std::atomic<bool> stop_search;

    void start_search(Position pos, SearchLimits limits);
}
#endif