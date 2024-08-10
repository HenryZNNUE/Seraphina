#pragma once

#include "types.h"

class Board;

namespace Seraphina
{
	namespace NNUE
	{
		struct alignas(64) Accumulator;
		struct alignas(64) AccumulatorRefreshTable;
	}

	int SimpleEval(Board& board);
	int Evaluate(Board& board, Move& move);
}