// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include "types.h"

class Board;
class MoveList;
struct Stack;

namespace Seraphina
{
	constexpr int PAWN_HISTORY_SIZE = 512;
	constexpr int CORRECTION_HISTORY_SIZE = 16384;

	typedef int16_t ButterflyHistory[2][64 * 64];
	typedef int16_t CapturedPieceToHistory[7][12][64];
	typedef int16_t ContinuationHistory[12][64][12][64];

	typedef int16_t CorrectionHistory[2][CORRECTION_HISTORY_SIZE];
	typedef int16_t NonPawnCorrectionHistory[CORRECTION_HISTORY_SIZE][2];
	typedef int16_t PieceToCorrectionHistory[12][64];
	typedef int16_t ContinuationCorrectionHistory[12][64][12][64];

	typedef int16_t LowPlyHistory[4][64 * 64];
	typedef int16_t PawnHistory[PAWN_HISTORY_SIZE][12][64];
	typedef int16_t PieceToHistory[12][64];

	enum Stages : int
	{
		// Main Moves
		TT_MOVE,

		QUIET_GEN,
		GOOD_QUIET,
		BAD_QUIET,

		CAPTURE_GEN,
		GOOD_CAPTURE,
		BAD_CAPTURE,

		// ProbCut Moves
		PROBCUT,

		// QSearch Moves
		QS,
		QS_CAPTURE
	};

	enum MovePickType : int
	{
		Next, Best
	};

	class MovePicker
	{
		const ButterflyHistory* mainHistory;
		const CapturedPieceToHistory* captureHistory;
		const PieceToHistory** continuationHistory;
		const PawnHistory* pawnHistory;
		MoveList* movelist;
		Move* current, *end, *endBad;
		Move ttMove, killermove;
		int depth, stage, threshold;
		// SearchStack* ss;

	public:
		template<MovePickType mpt>
		Move select();
		Move nextMove(Board& board, bool skipQuiets);
		template<MoveType mt>
		void score(Board& board);
	};
}