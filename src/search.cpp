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

constexpr int PieceValue[13] = { 208, 781, 825, 1276, 2538, 32001, 208, 781, 825, 1276, 2538, 32001, 0 };

int LMR[MAX_PLY][MAX_MOVES];
int LMP[2][MAX_PLY];

namespace Seraphina
{
	// https://en.cppreference.com/w/cpp/chrono/time_point/time_since_epoch
	Time now()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();
	}

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

		int swap = PieceValue[board.getBoard(to)] - threshold;

		if (swap < 0)
		{
			return false;
		}

		swap = PieceValue[board.getBoard(from)] - swap;

		if (swap <= 0)
		{
			return true;
		}

		int pov = board.get_pov();
		Bitboard occ = board.getoccBB(pov) ^ Seraphina::Bitboards::bit(from) ^ Seraphina::Bitboards::bit(to);
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

			if (!(povAttackers = (attackers & board.getoccBB(pov))))
			{
				break;
			}

			res ^= 1;

			if ((bb = povAttackers & board.getPieceBB(make_piece(pov, Seraphina::PieceList::PAWN))))
			{
				if ((swap = 208 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getBishopAttacks(to, occ) & x;
			}
			else if ((bb = povAttackers & board.getPieceBB(make_piece(pov, Seraphina::PieceList::KNIGHT))))
			{
				if ((swap = 781 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
			}
			else if ((bb = povAttackers & board.getPieceBB(make_piece(pov, Seraphina::PieceList::BISHOP))))
			{
				if ((swap = 825 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getBishopAttacks(to, occ) & x;
			}
			else if ((bb = povAttackers & board.getPieceBB(make_piece(pov, Seraphina::PieceList::ROOK))))
			{
				if ((swap = 1276 - swap) < res)
				{
					break;
				}

				occ ^= (bb & -bb);
				attackers |= Bitboards::getRookAttacks(to, occ) & v;
			}
			else if ((bb = povAttackers & board.getPieceBB(make_piece(pov, Seraphina::PieceList::QUEEN))))
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
				return (attackers & ~board.getoccBB(pov)) ? res ^ 1 : res;
			}
		}

		return bool(res);
	}

	// LMR & LMP are inspired by Berserk
	void Search::init()
	{

	}

	void Search::Root()
	{

	}

	int Search::search(int alpha, int beta, int d)
	{
		Stack stack[MAX_PLY + 10];
		Stack* ss = stack + 7;

		return 0;
	}

	int Search::qsearch(Stack& ss, int alpha, int beta, int d)
	{
		return 0;
	}

	void Search::StartThinking(Board& board)
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