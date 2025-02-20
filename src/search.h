// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include "types.h"
#include "movepick.h"

class Board;
class MoveList;
class TT;

namespace Seraphina
{
	struct RootMove
	{
		MoveList* pv;

		int seldepth = 0;
		int score = -VALUE_INFINITE;
		int prevScore = -VALUE_INFINITE;
		int avgScore = -VALUE_INFINITE;
	};

	struct Stack
	{
		Move move;
		int ply;
		Score staticEval;
		bool tt;

		PieceToHistory* continuationHistory;
		PieceToCorrectionHistory* continuationCorrectionHistory;
	};

	Time now();
	bool SEE(Board& board, Move& move, int threshold);

	class Search
	{
	private:

	public:
		ButterflyHistory butterflyHistory;
		CapturedPieceToHistory captureHistory;
		ContinuationHistory continuationHistory;

		CorrectionHistory pawncorrectionHistory;
		NonPawnCorrectionHistory nonpawnCorrectionHistory;
		ContinuationCorrectionHistory continuationCorrectionHistory;

		LowPlyHistory lowplyHistory;
		PawnHistory pawnHistory;

		void init();
		void Root();

		int search(int alpha, int beta, int d);
		int qsearch(Stack& ss, int alpha, int beta, int d);

		void StartThinking(Board& board);

		void UCIOutput();
		void PV();

		Search();
	};
}