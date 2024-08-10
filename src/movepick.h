#pragma once

#include "types.h"

class Board;
class MoveList;
struct SearchStack;

namespace Seraphina
{
	constexpr int PAWN_HISTORY_SIZE = 512;
	constexpr int CORRECTION_HISTORY_SIZE = 16384;

	typedef int16_t ButterflyHistory[2][64 * 64];
	typedef int16_t CapturedPieceToHistory[7][12][64];
	typedef int16_t ContinuationHistory[12][64];
	typedef int16_t CorrectionHistory[2][CORRECTION_HISTORY_SIZE];
	typedef int16_t CounterMoveHistory[12][64];
	typedef int16_t PawnHistory[PAWN_HISTORY_SIZE][12][64];
	typedef int16_t PieceToHistory[12][64];

	enum Stages
	{
		MAIN_TT, REFUTATION,
		GOOD_QUITE, BAD_QUITE,
		GOOD_CAPTURE, BAD_CAPTURE,
		EVASION_TT, EVASION,
		PROBCUT_TT, PROBCUT,
		QS_TT, QS_CAPTURE, QS_CHECK, QS_EVASION
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
		int depth, stage;
		// SearchStack* ss;

	public:
		Move nextMove();
		template<MoveType mt>
		void score(Board& board);
	};
}