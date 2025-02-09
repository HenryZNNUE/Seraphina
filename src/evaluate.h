// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

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
	void TraceEval(Board& board);
}