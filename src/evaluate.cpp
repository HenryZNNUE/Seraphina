#include "evaluate.h"
#include "bitboard.h"
#include "nnue.h"

namespace Seraphina
{
	int SimpleEval(Board& board)
	{
		return 208
			* (board.countPiece(PieceType::WHITE_PAWN) - board.countPiece(PieceType::BLACK_PAWN))
			+ (board.countPieceValues(Color::WHITE) - board.countPieceValues(Color::BLACK));
	}

	int Evaluate(Board& board, Move& move)
	{
		for (int pov = Color::WHITE; pov <= Color::BLACK; pov++)
		{
			if (!board.acc->computed[pov])
			{
				if (board.acc->updatable(board.acc, move, (Color)pov))
				{
					NNUE::update_accumulator(board, move, (Color)pov);
				}
				else
				{
					board.accRT->ApplyRefresh(board, (Color)pov);
				}
			}
		}

		int eval = NNUE::forward(*board.acc, board.currPOV());

		eval = (128 + SimpleEval(board)) * eval / 128;
		// eval += SimpleEval(board) * thread.comtempt[board.currPOV()] / 64;

		return std::clamp(eval, -31487, 31487);
	}
}