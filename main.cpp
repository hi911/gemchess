#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluate.h"
#include "zobrist.h"
#include "tt.h"

namespace Gemchess {
    // --- RELEASE CONFIGURATION ---
    const std::string NAME = "Gemchess v1.0"; 
    const std::string AUTHOR = "Google Gemini Pro & Cole Tarrant";

    class Engine {
    public:
        Engine() { 
            pos.set(START_FEN); 
            TT.resize(16); // Default Hash Size
        }

        int threadCount = 1; 

        void go(std::string params) {
            SearchLimits limits;
            limits.threads = threadCount;

            std::istringstream ss(params);
            std::string token;
            
            while (ss >> token) {
                if (token == "depth") ss >> limits.depth;
                else if (token == "wtime") ss >> limits.time[WHITE];
                else if (token == "btime") ss >> limits.time[BLACK];
                else if (token == "winc") ss >> limits.inc[WHITE];
                else if (token == "binc") ss >> limits.inc[BLACK];
                else if (token == "movetime") ss >> limits.movetime;
                else if (token == "infinite") limits.infinite = true;
            }

            if (searchThread.joinable()) searchThread.join();
            searchThread = std::thread(start_search, pos, limits);
        }

        void stop() {
            Gemchess::stop_search = true;
            if (searchThread.joinable()) searchThread.join();
        }

        void set_option(std::istringstream& is) {
            std::string token, name, value;
            is >> token; 
            while (is >> token && token != "value") name += (name.empty() ? "" : " ") + token;
            is >> value; 

            if (name == "Hash") {
                TT.resize(std::stoi(value));
            }
            else if (name == "Threads") {
                threadCount = std::stoi(value);
                if (threadCount < 1) threadCount = 1;
            }
        }

        void uci_new_game() {
            stop();
            TT.clear();
            states.clear();
            pos.set(START_FEN);
        }

        void set_position(std::istringstream& is) {
            std::string token, fen;
            is >> token;
            
            states.clear(); 

            if (token == "startpos") { fen = START_FEN; is >> token; }
            else if (token == "fen") { while (is >> token && token != "moves") fen += token + " "; }
            
            pos.set(fen);
            
            while (is >> token) {
                MoveList moves = generate_moves(pos);
                bool found = false;
                for (Move m : moves) {
                    std::string uci = "";
                    uci += char('a' + file_of(m.from_sq()));
                    uci += char('1' + rank_of(m.from_sq()));
                    uci += char('a' + file_of(m.to_sq()));
                    uci += char('1' + rank_of(m.to_sq()));
                    if (m.type() == PROMOTION) {
                        if (m.promotion_type() == QUEEN) uci += 'q';
                        else if (m.promotion_type() == ROOK) uci += 'r';
                        else if (m.promotion_type() == BISHOP) uci += 'b';
                        else if (m.promotion_type() == KNIGHT) uci += 'n';
                    }

                    if (token == uci) {
                        states.emplace_back(); 
                        pos.do_move(m, states.back());
                        found = true;
                        break;
                    }
                }
                if (!found) break; 
            }
        }

        void debug_print() { std::cout << pos; }

    private:
        Position pos;
        std::thread searchThread;
        std::deque<StateInfo> states; 
    };

    void uci_loop() {
        Engine engine;
        std::string line, token;
        std::cout << "id name " << NAME << "\nid author " << AUTHOR << std::endl;
        std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 128" << std::endl;
        std::cout << "uciok" << std::endl;

        while (std::getline(std::cin, line)) {
            std::istringstream is(line);
            token.clear();
            is >> std::skipws >> token;

            if (token == "quit") { engine.stop(); break; }
            else if (token == "isready") std::cout << "readyok" << std::endl;
            else if (token == "uci") std::cout << "uciok" << std::endl;
            else if (token == "ucinewgame") engine.uci_new_game();
            else if (token == "position") engine.set_position(is);
            else if (token == "setoption") engine.set_option(is);
            else if (token == "d") engine.debug_print();
            else if (token == "go") {
                std::string params;
                std::getline(is, params); 
                engine.go(params);
            }
            else if (token == "stop") engine.stop();
        }
    }
}

int main() {
    Gemchess::Bitboards::init();
    Gemchess::Zobrist::init(); 
    std::cout << Gemchess::NAME << " by " << Gemchess::AUTHOR << std::endl;
    Gemchess::uci_loop();
    return 0;
}
