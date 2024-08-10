// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "search.h"
#include "bitboard.h"
#include "movegen.h"
#include "tt.h"
#include "syzygy/tbprobe.h"

#include <math.h>

int LMR[MAX_PLY][MAX_MOVES];
int LMP[2][MAX_PLY];

namespace Seraphina
{
	bool SEE(Board& board, Move& move, int threshold)
	{
		MoveType mt = getMoveType(move);

		if (mt == MoveType::CAPTURE || mt == MoveType::ENPASSNT
			|| mt == MoveType::PROMOTION || mt == MoveType::PROMOTION_CAPTURE)
		{
			return Value::VALUE_ZERO >= threshold;
		}

		Square from = getFrom(move);
		Square to = getTo(move);

		int swap = board.getPieceValue(board.getBoard(to)) - threshold;

		if (swap < 0)
		{
			return false;
		}

		swap = board.getPieceValue(board.getBoard(from)) - swap;

		if (swap <= 0)
		{
			return true;
		}

		int pov = board.currPOV();
		Bitboard occ = board.getoccBB((Seraphina::Color)pov) ^ Seraphina::Bitboards::bit(from) ^ Seraphina::Bitboards::bit(to);
		Bitboard attackers = board.attackersTo(to, occ);
		Bitboard povAttackers, bb;
		int res = 1;

		const Bitboard x = board.getPieceBB(Seraphina::PieceType::WHITE_BISHOP)
			| board.getPieceBB(Seraphina::PieceType::BLACK_BISHOP)
			| board.getPieceBB(Seraphina::PieceType::WHITE_QUEEN)
			| board.getPieceBB(Seraphina::PieceType::BLACK_QUEEN);

		const Bitboard v = board.getPieceBB(Seraphina::PieceType::WHITE_ROOK)
			| board.getPieceBB(Seraphina::PieceType::BLACK_ROOK)
			| board.getPieceBB(Seraphina::PieceType::WHITE_QUEEN)
			| board.getPieceBB(Seraphina::PieceType::BLACK_QUEEN);

		while (true)
		{
			pov = ~pov;
			attackers &= occ;

			if (!(povAttackers = (attackers & board.getoccBB((Seraphina::Color)pov))))
			{
				break;
			}

			res ^= 1;

			if (bb = povAttackers & board.getPieceBB(makepiece((Seraphina::Color)pov, Seraphina::PieceList::PAWN)))
			{
				if ((swap = 208 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getBishopAttacks(to, occ) & x;
			}
			else if (bb = povAttackers & board.getPieceBB(makepiece((Seraphina::Color)pov, Seraphina::PieceList::KNIGHT)))
			{
				if ((swap = 781 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
			}
			else if (bb = povAttackers & board.getPieceBB(makepiece((Seraphina::Color)pov, Seraphina::PieceList::BISHOP)))
			{
				if ((swap = 825 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getBishopAttacks(to, occ) & x;
			}
			else if (bb = povAttackers & board.getPieceBB(makepiece((Seraphina::Color)pov, Seraphina::PieceList::ROOK)))
			{
				if ((swap = 1276 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getRookAttacks(to, occ) & v;
			}
			else if (bb = povAttackers & board.getPieceBB(makepiece((Seraphina::Color)pov, Seraphina::PieceList::QUEEN)))
			{
				if ((swap = 2538 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= (Bitboards::getBishopAttacks(to, occ) & x) | (Bitboards::getRookAttacks(to, occ) & v);
			}
			else
			{
				return (attackers & ~board.getoccBB((Seraphina::Color)pov)) ? res ^ 1 : res;
			}
		}

		return bool(res);
	}

	// LMR & LMP are inspired by Berserk
	void Search::init()
	{
		move = depth = seldepth = nodes = score = previousScore = avgScore = multipv = time = 0;
		pv = NULL;

		for (int i = 0; i < MAX_PLY; i++)
		{
			for (int j = 0; j < MAX_MOVES; j++)
			{
				LMR[i][j] = log(i) * log(j) / 2.0385 + 0.2429;
			}

			LMP[0][i] = 1.3050 + 0.3503 * i * i;
			LMP[1][i] = 2.1885 + 0.9911 * i * i;
		}

		LMR[0][0] = LMR[0][1] = LMR[1][0] = 0;
	}

	void Search::Root()
	{

	}

	int Search::alpha_beta(int alpha, int beta, int depth)
	{

	}

	int Search::qsearch()
	{

	}

	void Search::StartThinking(Board& board)
	{

	}

	void Search::SearchClear()
	{

	}

	void Search::UCIOutput()
	{

	}

	void Search::PV()
	{

	}

	Search::Search()
	{
		init();
	}
}