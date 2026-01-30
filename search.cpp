#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include <chrono>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>

namespace Gemchess {

    std::atomic<bool> stop_search(false);
    uint64_t stopTime = 0; 
    std::atomic<long long> global_nodes(0);

    // --- HELPER: GET PV LINE (FIXED) ---
    void get_pv_line(Position& pos, std::vector<Move>& line) {
        // FIXED: Use a vector to store a unique StateInfo for each move.
        // We reserve space to prevent reallocation, which would invalidate pointers.
        std::vector<StateInfo> states;
        states.reserve(64); 

        Move bestMove = TT.probe_move(pos.key());
        int count = 0;
        
        while (bestMove.raw() != 0 && count < 20) {
            bool legal = false;
            MoveList moves = generate_moves(pos);
            for(Move m : moves) {
                if (m == bestMove) { legal = true; break; }
            }
            if (!legal) break;

            line.push_back(bestMove);
            
            // Create a new state in the vector and use it
            states.emplace_back(); 
            pos.do_move(bestMove, states.back());
            
            bestMove = TT.probe_move(pos.key());
            count++;
        }
        
        // Undo moves to restore the board to the original position
        for (int i = line.size() - 1; i >= 0; --i) {
            pos.undo_move(line[i]);
        }
    }

    struct SearchContext {
        int thread_id;
        long long nodes = 0;
        Move Killers[MAX_PLY][2];
        int History[2][64][64];

        SearchContext(int id) : thread_id(id) {
            std::memset(Killers, 0, sizeof(Killers));
            std::memset(History, 0, sizeof(History));
        }

        void score_moves(MoveList& list, const Position& pos, int ply) {
            const int SortValue[] = { 0, 100, 300, 300, 500, 900, 10000 };
            
            for (int i = 0; i < list.count; ++i) {
                Move m = list.moves[i];
                if (pos.piece_on(m.to_sq()) != NO_PIECE) {
                    int victim = SortValue[type_of(pos.piece_on(m.to_sq()))];
                    int attacker = SortValue[type_of(pos.piece_on(m.from_sq()))];
                    list.scores[i] = 30000 + victim * 10 - attacker; 
                }
                else if (m.type() == PROMOTION) {
                    list.scores[i] = 30000 + SortValue[m.promotion_type()];
                }
                else if (ply < MAX_PLY && m == Killers[ply][0]) list.scores[i] = 29000;
                else if (ply < MAX_PLY && m == Killers[ply][1]) list.scores[i] = 28000;
                else {
                    list.scores[i] = History[pos.side_to_move()][m.from_sq()][m.to_sq()];
                }
            }
            list.sort();
        }

        int qsearch(Position& pos, int alpha, int beta) {
            if ((nodes & 2047) == 0 && stop_search) return 0;
            nodes++;

            int stand_pat = evaluate(pos);
            if (stand_pat >= beta) return beta;
            if (alpha < stand_pat) alpha = stand_pat;

            MoveList moves = generate_moves(pos);
            score_moves(moves, pos, MAX_PLY);

            for (Move m : moves) {
                bool isCapture = (pos.piece_on(m.to_sq()) != NO_PIECE) || (m.type() == EN_PASSANT);
                bool isProm = (m.type() == PROMOTION);
                if (!isCapture && !isProm) continue;

                StateInfo st;
                pos.do_move(m, st);
                if (pos.attackers_to(pos.king_square(~pos.side_to_move()), pos.side_to_move())) {
                    pos.undo_move(m);
                    continue;
                }

                int score = -qsearch(pos, -beta, -alpha);
                pos.undo_move(m);

                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }
            return alpha;
        }

        int negamax(Position& pos, int depth, int alpha, int beta, int ply, bool do_null) {
            if ((nodes & 2047) == 0) {
                if (stop_search) return 0;
                if (thread_id == 0 && stopTime > 0 && now() >= stopTime) stop_search = true;
            }

            Move ttMove = Move::none();
            int ttScore;
            if (TT.probe(pos.key(), ttScore, depth, alpha, beta, ttMove)) {
                if (ply > 0) return ttScore; 
            }

            bool inCheck = pos.attackers_to(pos.king_square(pos.side_to_move()), ~pos.side_to_move());
            int extension = inCheck ? 1 : 0;
            if (depth + extension <= 0) return qsearch(pos, alpha, beta);

            nodes++;

            if (do_null && depth >= 3 && !inCheck && ply > 0) {
                StateInfo st;
                pos.do_null_move(st);
                int score = -negamax(pos, depth - 1 - 2, -beta, -beta + 1, ply + 1, false);
                pos.undo_null_move();
                if (stop_search) return 0;
                if (score >= beta) return beta;
            }

            MoveList moves = generate_moves(pos);
            score_moves(moves, pos, ply);
            
            if (ttMove.raw() != 0) {
                for(int i=0; i<moves.count; ++i) {
                    if (moves.moves[i] == ttMove) {
                        moves.scores[i] = 40000;
                        break;
                    }
                }
                moves.sort();
            }
            
            int legalMoves = 0;
            int bestScore = -INF;
            Move bestMove = Move::none();
            int startAlpha = alpha;

            for (int i = 0; i < moves.count; ++i) {
                Move m = moves.moves[i];
                StateInfo st;
                pos.do_move(m, st);

                if (pos.attackers_to(pos.king_square(~pos.side_to_move()), pos.side_to_move())) {
                    pos.undo_move(m);
                    continue;
                }
                legalMoves++;

                int score;
                int newDepth = depth - 1 + extension;

                if (i == 0) {
                    score = -negamax(pos, newDepth, -beta, -alpha, ply + 1, true);
                } else {
                    int reduction = 0;
                    bool isKiller = (ply < MAX_PLY) && (m == Killers[ply][0] || m == Killers[ply][1]);
                    bool isCapture = (pos.piece_on(m.to_sq()) != NO_PIECE) || (m.type() == EN_PASSANT);
                    
                    if (depth >= 3 && !isCapture && !isKiller && !inCheck && i > 3) {
                        reduction = 1;
                        if (i > 8) reduction = 2;
                    }

                    score = -negamax(pos, newDepth - reduction, -alpha - 1, -alpha, ply + 1, true);

                    if (score > alpha && score < beta) {
                        if (reduction > 0) score = -negamax(pos, newDepth, -alpha - 1, -alpha, ply + 1, true);
                        if (score > alpha && score < beta) score = -negamax(pos, newDepth, -beta, -alpha, ply + 1, true);
                    }
                }

                pos.undo_move(m);
                if (stop_search) return 0;

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = m;
                }

                if (score >= beta) {
                    TT.save(pos.key(), score, depth, TT_BETA, m);
                    bool isCapture = (pos.piece_on(m.to_sq()) != NO_PIECE);
                    if (!isCapture) {
                        if (ply < MAX_PLY) {
                            Killers[ply][1] = Killers[ply][0];
                            Killers[ply][0] = m;
                        }
                        int& hist = History[pos.side_to_move()][m.from_sq()][m.to_sq()];
                        hist += depth * depth;
                        if (hist > 20000) hist = 20000;
                    }
                    return beta; 
                }
                if (score > alpha) alpha = score;
            }

            if (legalMoves == 0) {
                if (inCheck) return -MATE + ply; 
                else return 0;
            }

            TTFlag flag = (bestScore > startAlpha) ? TT_EXACT : TT_ALPHA;
            TT.save(pos.key(), bestScore, depth, flag, bestMove);
            return bestScore;
        }
    };

    void worker_loop(Position pos, int maxDepth, int id) {
        SearchContext ctx(id);
        for (int depth = 1; depth <= maxDepth; ++depth) {
            ctx.negamax(pos, depth, -INF, INF, 0, true);
            if (stop_search) break;
        }
        global_nodes += ctx.nodes;
    }

    void start_search(Position pos, SearchLimits limits) {
        stop_search = false;
        stopTime = 0;
        global_nodes = 0;

        int us = pos.side_to_move();
        if (limits.time[us] > 0) {
            int time_allotted = (limits.time[us] / 20) + (limits.inc[us] / 2);
            stopTime = now() + time_allotted;
        } else if (limits.movetime > 0) {
            stopTime = now() + limits.movetime;
        }

        int maxDepth = (limits.depth == -1) ? 64 : limits.depth; 
        
        std::vector<std::thread> threads;
        if (limits.threads > 1) {
            for (int i = 1; i < limits.threads; ++i) {
                threads.emplace_back(worker_loop, pos, maxDepth, i);
            }
        }

        SearchContext mainCtx(0);
        Move overallBestMove = Move::none();

        for (int depth = 1; depth <= maxDepth; ++depth) {
            int alpha = -INF;
            int beta = INF;
            if (depth > 4) {
                alpha = -20000; 
                beta = 20000;
            }

            MoveList moves = generate_moves(pos);
            mainCtx.score_moves(moves, pos, 0);
            
            if (overallBestMove.raw() != 0) {
                for(int i=0; i<moves.count; ++i) {
                    if (moves.moves[i] == overallBestMove) {
                        moves.scores[i] = 50000; 
                        break;
                    }
                }
                moves.sort();
            }

            int bestScore = -INF;
            Move currentBestMove = Move::none();
            
            for (int i = 0; i < moves.count; ++i) {
                Move m = moves.moves[i];
                StateInfo st;
                pos.do_move(m, st);
                
                if (pos.attackers_to(pos.king_square(~pos.side_to_move()), pos.side_to_move())) {
                    pos.undo_move(m);
                    continue;
                }

                int score;
                if (i == 0) score = -mainCtx.negamax(pos, depth - 1, -beta, -alpha, 1, true);
                else {
                    score = -mainCtx.negamax(pos, depth - 1, -alpha - 1, -alpha, 1, true);
                    if (score > alpha) score = -mainCtx.negamax(pos, depth - 1, -beta, -alpha, 1, true);
                }

                pos.undo_move(m);
                if (stop_search) break;

                if (score > bestScore) {
                    bestScore = score;
                    currentBestMove = m;
                }
                if (score > alpha) alpha = score;
            }

            if (stop_search) break;

            if (currentBestMove.raw() != 0) {
                overallBestMove = currentBestMove;
                TT.save(pos.key(), bestScore, depth, TT_EXACT, overallBestMove);
            }

            std::vector<Move> pvLine;
            get_pv_line(pos, pvLine);

            auto ms = now(); (void)ms;
            std::cout << "info depth " << depth << " score cp " << bestScore 
                      << " nodes " << mainCtx.nodes << " pv ";
            for (Move m : pvLine) {
                std::cout << char('a' + file_of(m.from_sq())) << char('1' + rank_of(m.from_sq()))
                          << char('a' + file_of(m.to_sq())) << char('1' + rank_of(m.to_sq())) << " ";
            }
            std::cout << std::endl;
        }

        stop_search = true;
        for (auto& t : threads) t.join();

        if (overallBestMove.raw() == 0) {
            MoveList moves = generate_moves(pos);
            for(Move m : moves) { 
                StateInfo st; pos.do_move(m, st);
                if (!pos.attackers_to(pos.king_square(~pos.side_to_move()), pos.side_to_move())) {
                    overallBestMove = m; pos.undo_move(m); break;
                }
                pos.undo_move(m);
            }
        }

        std::cout << "bestmove " 
                  << char('a' + file_of(overallBestMove.from_sq())) << char('1' + rank_of(overallBestMove.from_sq()))
                  << char('a' + file_of(overallBestMove.to_sq())) << char('1' + rank_of(overallBestMove.to_sq())) 
                  << std::endl;
    }
}