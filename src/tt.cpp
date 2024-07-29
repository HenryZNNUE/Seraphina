// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "tt.h"

namespace Seraphina
{
	TTEntry* TT::getTTptr(const U64 key)
	{
		// Pointer ttprt serves as extracting the address of ttentry[ hash % ttentry.size()]
		TTEntry* ttptr = &ttentry[key % ttentry.size()];

		if (ttptr->key != key) // If ttptr's hash doesn't match the give hash
		{
			return nullptr; // Not the same hash
		}

		return ttptr; // Return the pointer
	}

	void TT::updateTT(const TTEntry& tte)
	{
		if (tte.depth >= ttentry[tte.key % ttentry.size()].depth) // If current depth is bigger or equal to tt history depth
		{
			ttentry[tte.key % ttentry.size()] = tte; // Update tt
		}
	}

	// Init TT
	void TT::initTT()
	{
		std::fill(ttentry.begin(), ttentry.end(), TTEntry());
	}

	// Aims at altering the size of the tt vector
	// If the give tt size is smaller than current tt size, then erase extra elements
	// If the give tt size is bigger than current tt size, then allocate a set of memory
	void TT::TTDimentionUpate(size_t size)
	{
		ttentry.resize(size);
	}

}