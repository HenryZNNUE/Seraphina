// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

// TTEntry struct is the 10 bytes transposition table entry, defined as below:
//
// key        16 bit
// depth       8 bit
// generation  5 bit
// pv node     1 bit
// bound type  2 bit
// move       16 bit
// value      16 bit
// eval value 16 bit

#include <vector>

#include "types.h"

namespace Seraphina
{
	// Transposition Table Flag
	enum TT_FLAG
	{
		NO_FLAG, BEST, LOWER, UPPER
	};

	// Transposition Table Entry
	class TTEntry
	{
	public:
		U64 key = 0ULL;
		uint8_t depth = 0;
		Move best_move = 0;
		uint16_t eval = 0;
		TT_FLAG ttflag = NO_FLAG;

		TTEntry() = default;
		TTEntry(U64 k, uint8_t d, const Move& bm, uint16_t e, TT_FLAG tf) :
			key(k), depth(d), best_move(bm), eval(e), ttflag(tf) {}
	};

	// Transposition Table Main
	class TT
	{
	private:
		std::vector<TTEntry> ttentry;

	public:
		TT(size_t size) : ttentry(size) {}

		TTEntry* getTTptr(const U64 key);

		void updateTT(const TTEntry& tte);

		// Init TT
		void initTT();

		// Aims at altering the size of the tt vector
		// If the give tt size is smaller than current tt size, then erase extra elements
		// If the give tt size is bigger than current tt size, then allocate a set of memory
		void TTDimentionUpate(size_t size);
	};

	// The "extern" here makes the "TT" class GLOBAL in the project,
	// which avoids Syntax Error "Redefinition" when calling the tt.h header file
	extern TT tt;
}