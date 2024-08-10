#pragma once

// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "types.h"

class Board;
class MoveList;
class TT;

typedef int16_t PieceToHistory[12][64];

namespace Seraphina
{
	bool SEE(Board& board, Move& move, int threshold);

	class Search
	{
	private:
		Move move;
		int depth, seldepth, nodes;
		Score score, previousScore, avgScore;
		MoveList* pv;
		int multipv;
		int time;

	public:
		TT* tt;
		void init();
		void Root();

		int alpha_beta(int alpha, int beta, int depth);
		int qsearch();

		void StartThinking(Board& board);
		void SearchClear();

		void UCIOutput();
		void PV();

		Search();
	};

	struct SearchStack
	{
		int staticEval, ply, statScore;
		Move* pv;
		Move currentMove, excludedMove, killers[2];
		PieceToHistory** continuationHistory;
		bool ttPV;
	};
}