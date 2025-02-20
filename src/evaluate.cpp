// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "evaluate.h"
#include "bitboard.h"
#include "nnue.h"
#include "uci.h"

namespace Seraphina
{
	int SimpleEval(Board& board)
	{
		return 208
			* (board.getPieceCount(PieceType::WHITE_PAWN) - board.getPieceCount(PieceType::BLACK_PAWN))
			+ (board.getPieceValues(Color::WHITE) - board.getPieceValues(Color::BLACK));
	}

	int Evaluate(Board& board, Move& move)
	{
		for (int pov = Color::WHITE; pov <= Color::BLACK; ++pov)
		{
			if (!board.acc->computed[pov])
			{
				if (board.acc->updatable(board.acc, move, pov))
				{
					NNUE::update_accumulator(board, move, pov);
				}
				else
				{
					board.accRT->ApplyRefresh(board, pov);
				}
			}
		}

		int eval = NNUE::forward(*board.acc, board.get_pov());

		eval = (131 * eval - 5 * SimpleEval(board)) / 128;
		eval = (eval / 16) * 16 - 1 + (board.getZobrist() & 2); // tests should be made to see the gain
		eval -= eval * board.getFifty() / 128;

		return std::clamp(eval, -31486, 31486);
	}

	void TraceEval(Board& board)
	{
		board.acc = static_cast<NNUE::Accumulator*>(NNUE::aligned_malloc(sizeof(NNUE::Accumulator), 64));
		Color pov = board.get_pov();

		NNUE::reset_accumulator(board, Color::WHITE);
		NNUE::reset_accumulator(board, Color::BLACK);

		int eval = NNUE::forward(*board.acc, pov);

		std::cout << "NNUE Evaluation " << cp(board, eval) * 0.01 << "\n";

		eval = (131 * eval - 5 * SimpleEval(board)) / 128;
		eval = (eval / 16) * 16 - 1 + (board.getZobrist() & 2);
		eval -= eval * board.getFifty() / 256;
		eval = std::clamp(eval, -31486, 31486);
		eval = pov == Color::WHITE ? eval : -eval;

		std::cout << "Final Evaluation " << cp(board, eval) * 0.01 << "\n";

		free(board.acc);
	}
}