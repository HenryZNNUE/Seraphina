// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "movegen.h"
#include "bitboard.h"
#include "nnue.h"

const char* SQstr[SQ_NUM] = {
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
		"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
		"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
		"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
		"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
		"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
		"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
		"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

const char* Promostr = "-nbrq-";

namespace Seraphina
{
	void makemove(Board& board, const Move& move, bool update = true)
	{
		BoardInfo history = *board.getBoardInfo();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		MoveType mt = getMoveType(move);
		Color pov = board.currPOV();

		board.setFiftyIncremental();

		(mt == MoveType::CAPTURE
			|| mt == MoveType::ENPASSNT
			|| mt == MoveType::PROMOTION_CAPTURE)
			? board.replacePiece(to, pt)
			: board.setPiece(to, pt);

		if (mt == MoveType::SHORT_CASTLE || mt == MoveType::LONG_CASTLE)
		{
			board.castling &= board.castling_rights[from];
			board.castling &= board.castling_rights[to];
		}

		/*
		board.FlipPieceBB(pt, from, to);
		board.FlipOccBB(pov, from, to);
		board.FlipOccBB(Color::NO_COLOR, from, to);
		*/

		if (mt == MoveType::NORMAL)
		{
			board.removePiece(from);
			board.setPiece(to, pt);

			if (getpiece(pt) == PieceList::PAWN)
			{
				board.setFifty(0);
			}
		}

		if (mt == MoveType::SHORT_CASTLE)
		{
			if (pov == Color::WHITE)
			{
				/*
				board.FlipPieceBB(PieceType::WHITE_ROOK, Square::SQ_H1, Square::SQ_F1);
				board.FlipOccBB(pov, Square::SQ_H1, Square::SQ_F1);
				board.FlipOccBB(Color::NO_COLOR, Square::SQ_H1, Square::SQ_F1);

				if (board.getBoardInfo()->chess960)
				{
					board.removePiece(Square::SQ_H1);
				}
				*/

				board.setPiece(Square::SQ_F1, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_H1);
			}
			else
			{
				board.setPiece(Square::SQ_F8, PieceType::BLACK_ROOK);
				board.removePiece(Square::SQ_H8);
			}
		}

		if (mt == MoveType::LONG_CASTLE)
		{
			if (pov == Color::WHITE)
			{
				board.setPiece(Square::SQ_D1, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_A1);
			}
			else
			{
				board.setPiece(Square::SQ_D8, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_A8);
			}
		}

		if (mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE)
		{
			board.getBoardInfo()->capture = board.getBoard(to);
			board.removePiece(from);
			board.removePiece(to);
			board.setPiece(to, (mt == MoveType::PROMOTION_CAPTURE) ? getCapPromo(move) : pt);

			board.setFifty(0);
		}

		if (mt == MoveType::ENPASSNT)
		{
			int ENPASSNT = to - (pov == Color::WHITE ? Direction::NORTH : Direction::SOUTH);

			if (Bitboards::getPawnAttacks(pov, (Square)ENPASSNT) & board.getPieceBB(makepiece((Color)~pov, PieceList::PAWN)))
			{
				history.ENPASSNT = ENPASSNT;
			}

			board.removePiece(from);
			board.removePiece((Square)ENPASSNT);
			board.setPiece(to, pt);

			board.setFifty(0);
		}

		if (mt == MoveType::PROMOTION)
		{
			board.removePiece(from);
			board.setPiece(to, getCapPromo(move));

			board.setFifty(0);
		}

		history.repetition = 0;
		int end = std::min(board.getFifty(), board.getpliesFromNull());
		BoardInfo prev = *(board.getHistory().rend() + 1);

		if (end >= 4)
		{
			for (int i = 4; i <= end; i += 2)
			{
				if (history.zobrist == prev.zobrist)
				{
					history.repetition = prev.repetition ? -i : i;
				}
			}
		}

		board.setPOV((Color)(pov ^ 1));
		board.setMoveNumIncremental();
		board.setpliesFromNullIncremental();
		board.setHistoryIncremental(history);
		board.updateZobrist(move);

		board.setCPK();
		board.setThreats();

		if (update)
		{
			board.acc->computed[Color::WHITE] = board.acc->computed[Color::BLACK] = false;
		}
	}

	void undomove(Board& board, const Move& move)
	{
		BoardInfo history = *board.getBoardInfo();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		MoveType mt = getMoveType(move);
		int pov = board.currPOV();
		int prevPOV = pov ^ 1;
		board.setPOV((Color)prevPOV);
		board.acc--;
		board.undoMoveNumIncremental();
		board.setpliesFromNull(0);
		board.undoHistoryIncremental();

		if (mt == MoveType::NORMAL)
		{
			board.removePiece(to);
			board.setPiece(from, pt);
		}

		if (mt == MoveType::SHORT_CASTLE)
		{
			if (prevPOV == Color::WHITE)
			{
				board.setPiece(Square::SQ_H1, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_F1);
			}
			else
			{
				board.setPiece(Square::SQ_H8, PieceType::BLACK_ROOK);
				board.removePiece(Square::SQ_F8);
			}
		}

		if (mt == MoveType::LONG_CASTLE)
		{
			if (prevPOV == Color::WHITE)
			{
				board.setPiece(Square::SQ_A1, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_D1);
			}
			else
			{
				board.setPiece(Square::SQ_A8, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_D8);
			}
		}

		if (mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE)
		{
			board.removePiece(to);
			board.setPiece(from, pt);
			board.setPiece(to, (PieceType)history.capture);
		}

		if (mt == MoveType::ENPASSNT)
		{
			board.removePiece((Square)history.ENPASSNT);
			board.setPiece(from, pt);
			board.setPiece((Square)history.ENPASSNT, makepiece((Color)pov, PieceList::PAWN));
		}

		if (mt == MoveType::PROMOTION)
		{
			board.removePiece(to);
			board.setPiece(from, pt);
		}
	}

	inline char movetoString(Board& board, const Move& move)
	{
		static char output[6];
		Square from = getFrom(move);
		Square to = getTo(move);
		MoveType mt = getMoveType(move);

		if (board.isChess960() && (mt == MoveType::SHORT_CASTLE || mt == MoveType::LONG_CASTLE))
		{
			if (board.currPOV() == Color::WHITE)
			{
				mt == MoveType::SHORT_CASTLE
					? to = Square::SQ_F1
					: to = Square::SQ_D1;
			}
			else
			{
				mt == MoveType::SHORT_CASTLE
					? to = Square::SQ_F8
					: to = Square::SQ_D8;
			}
		}

		if (mt == MoveType::PROMOTION || mt == MoveType::PROMOTION_CAPTURE)
		{
			sprintf(output, "%s%s%c", SQstr[from], SQstr[to], Promostr[getpiece(getCapPromo(move))]);
		}
		else
		{
			sprintf(output, "%s%s", SQstr[from], SQstr[to]);
		}
	}
	
	// Inspired by Berserk
	bool isPseudoLegal(Board& board, const Move& move)
	{
		Color pov = board.currPOV();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		PieceList p = getpiece(pt);
		MoveType mt = getMoveType(move);

		if (!move || getcolor(pt) != pov || pt != board.getBoard(from))
		{
			return false;
		}

		if (mt == MoveType::SHORT_CASTLE)
		{
			Bitboard kc = board.getSQbtw(from, to) | Bitboards::bit(to);

			if (pov == Color::WHITE)
			{
				if (board.getBoard(Square::SQ_F1) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_G1) == PieceType::NO_PIECETYPE)
				{
					if (kc & board.getThreatened())
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (board.getBoard(Square::SQ_F8) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_G8) == PieceType::NO_PIECETYPE)
				{
					if (kc & board.getThreatened())
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				else
				{
					return false;
				}
			}
		}

		if (mt == MoveType::LONG_CASTLE)
		{
			Bitboard kc = board.getSQbtw(from, to) | Bitboards::bit(to);

			if (pov == Color::WHITE)
			{
				if (board.getBoard(Square::SQ_B1) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_C1) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_D1) == PieceType::NO_PIECETYPE)
				{
					if (kc & board.getThreatened())
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (board.getBoard(Square::SQ_B8) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_C8) == PieceType::NO_PIECETYPE
					&& board.getBoard(Square::SQ_D8) == PieceType::NO_PIECETYPE)
				{
					if (kc & board.getThreatened())
					{
						return false;
					}
					else
					{
						return true;
					}
				}
				else
				{
					return false;
				}
			}
		}

		if (mt == MoveType::PROMOTION)
		{
			if (Bitboards::popcount(board.getCheckers()) > 1)
			{
				return false;
			}

			Bitboard checkers = board.getCheckers();
			Bitboard valid = checkers
				? (checkers | board.getSQbtw(board.getKingSquare(pov), Bitboards::lsb(checkers)))
				: -1ULL;
			Bitboard bottom = (pov == Color::WHITE ? Rank8BB : Rank1BB);
			Bitboard opts = (pov == Color::WHITE
				? Bitboards::shift<Direction::NORTH>(from & ~board.getoccBB(Color::NO_COLOR))
				| (Bitboards::getPawnAttacks(pov, from) & board.getoccBB(Color::BLACK))
				: Bitboards::shift<Direction::SOUTH>(from & ~board.getoccBB(Color::NO_COLOR))
				| (Bitboards::getPawnAttacks(pov, from) & board.getoccBB(Color::WHITE)));

			return !!((bottom & opts & valid) & to);
		}

		if (p == PieceList::PAWN)
		{
			if ((Rank8BB | Rank1BB) & to)
			{
				return false;
			}

			if (mt != MoveType::ENPASSNT)
			{
				if (!(Bitboards::getPawnAttacks(pov, from) & board.getoccBB((Color)(pov ^ 1)) & to)
					&& !((from + (pov == Color::WHITE ? Direction::NORTH : Direction::SOUTH) == to) && board.getBoard(to) == PieceType::NO_PIECETYPE)
					&& !((from + ((pov == Color::WHITE ? Direction::NORTH : Direction::SOUTH) << 1) == to)
						&& ((from ^ (pov * 7)) == 1) && board.getBoard(to) == PieceType::NO_PIECETYPE && board.getBoard((Square)((U64)to - (pov == Color::WHITE ? Direction::NORTH : Direction::SOUTH))) == PieceType::NO_PIECETYPE))
				{
					return false;
				}
			}
		}
		else if (!(Bitboards::getPieceAttacks(from, board.getoccBB(Color::NO_COLOR), p) & to))
		{
			return false;
		}

		if ((mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE) && mt != MoveType::ENPASSNT
			&& board.getBoard(to) == PieceType::NO_PIECETYPE)
		{
			return false;
		}

		if (mt != MoveType::CAPTURE && board.getBoard(to) != PieceType::NO_PIECETYPE)
		{
			return false;
		}

		if (mt == MoveType::ENPASSNT && to != board.getBoardInfo()->ENPASSNT)
		{
			return false;
		}

		if (p == PieceList::KING && board.isAttacked(pov, to))
		{
			return false;
		}

		if (board.getCheckers())
		{
			if (p != PieceList::KING)
			{
				if (Bitboards::popcount(board.getCheckers()) > 1)
				{
					return false;
				}

				if (!(board.getSQbtw(board.getKingSquare(pov), Bitboards::lsb(board.getCheckers())) & to))
				{
					return false;
				}
			}
			else if (!(board.attackersTo(to, board.getoccBB(Color::NO_COLOR)) & to))
			{
				return false;
			}
		}

		return true;
	}
}

template <Seraphina::PieceList p>
void MoveList::generatebyPieceType(Board& board, Seraphina::MoveType mt, Bitboard& opts)
{
	if (p == Seraphina::PieceList::PAWN)
	{
		// Generate all pawn moves
		if (board.currPOV() == Seraphina::Color::WHITE)
		{
			if (mt == Seraphina::MoveType::NORMAL)
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::WHITE_PAWN);
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH>(pawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR);
				Bitboard dp = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH>(target) & ~board.getoccBB(Seraphina::Color::NO_COLOR);

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to - Seraphina::Direction::NORTH), to, Seraphina::PieceType::WHITE_PAWN, mt));
				}

				while (dp)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(dp);
					moves.emplace_back(setMove((Seraphina::Square)(to - (Seraphina::Direction::NORTH << 1)), to, Seraphina::PieceType::WHITE_PAWN, mt));
				}
			}

			else if (mt == Seraphina::MoveType::CAPTURE)
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::WHITE_PAWN) & ~Rank7BB;
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_EAST>(pawn) & board.getoccBB(Seraphina::Color::BLACK) & opts;
				Bitboard target2 = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_WEST>(pawn) & board.getoccBB(Seraphina::Color::BLACK) & opts;

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_WEST), to, Seraphina::PieceType::WHITE_PAWN, mt, board.getBoard(to)));
				}

				while (target2)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target2);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_EAST), to, Seraphina::PieceType::WHITE_PAWN, mt, board.getBoard(to)));
				}

				if (board.getBoardInfo()->ENPASSNT)
				{
					Bitboard movers = Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::WHITE, (Seraphina::Square)board.getBoardInfo()->ENPASSNT) & pawn;

					while (movers)
					{
						Seraphina::Square from = Seraphina::Bitboards::poplsb(movers);
						moves.emplace_back(setMove(from, (Seraphina::Square)board.getBoardInfo()->ENPASSNT, Seraphina::PieceType::WHITE_PAWN, mt, Seraphina::PieceType::BLACK_PAWN));
					}
				}
			}

			else
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::WHITE_PAWN) & Rank7BB;
				Bitboard ptarget = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH>(pawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR) & opts;
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_EAST>(pawn) & board.getoccBB(Seraphina::Color::BLACK) & opts;
				Bitboard target2 = Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_WEST>(pawn) & board.getoccBB(Seraphina::Color::BLACK) & opts;

				while (ptarget)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(ptarget);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::WHITE_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::WHITE_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::WHITE_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::WHITE_QUEEN));
				}

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_WEST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_WEST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_WEST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_WEST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_QUEEN));
				}

				while (target2)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target2);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_EAST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_EAST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_EAST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::SOUTH_EAST), to, Seraphina::PieceType::WHITE_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::WHITE_QUEEN));
				}
			}
		}
		else
		{
			if (mt == Seraphina::MoveType::NORMAL)
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::BLACK_PAWN);
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH>(pawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR);
				Bitboard dp = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH>(target) & ~board.getoccBB(Seraphina::Color::NO_COLOR);

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to - Seraphina::Direction::SOUTH), to, Seraphina::PieceType::BLACK_PAWN, mt));
				}

				while (dp)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(dp);
					moves.emplace_back(setMove((Seraphina::Square)(to + (Seraphina::Direction::NORTH << 1)), to, Seraphina::PieceType::BLACK_PAWN, mt));
				}
			}

			else if (mt == Seraphina::MoveType::CAPTURE)
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::BLACK_PAWN) & ~Rank2BB;
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_EAST>(pawn) & board.getoccBB(Seraphina::Color::WHITE) & opts;
				Bitboard target2 = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_WEST>(pawn) & board.getoccBB(Seraphina::Color::WHITE) & opts;

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_WEST), to, Seraphina::PieceType::BLACK_PAWN, mt, board.getBoard(to)));
				}

				while (target2)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target2);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_EAST), to, Seraphina::PieceType::BLACK_PAWN, mt, board.getBoard(to)));
				}
			}

			else if (mt == Seraphina::MoveType::ENPASSNT)
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::BLACK_PAWN) & ~Rank2BB;

				if (board.getBoardInfo()->ENPASSNT)
				{
					Bitboard movers = Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::BLACK, (Seraphina::Square)board.getBoardInfo()->ENPASSNT) & pawn;

					while (movers)
					{
						Seraphina::Square from = Seraphina::Bitboards::poplsb(movers);
						moves.emplace_back(setMove(from, (Seraphina::Square)board.getBoardInfo()->ENPASSNT, Seraphina::PieceType::BLACK_PAWN, mt, Seraphina::PieceType::WHITE_PAWN));
					}
				}
			}

			else
			{
				Bitboard pawn = board.getPieceBB(Seraphina::PieceType::BLACK_PAWN) & Rank7BB;
				Bitboard ptarget = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH>(pawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR) & opts;
				Bitboard target = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_EAST>(pawn) & board.getoccBB(Seraphina::Color::WHITE) & opts;
				Bitboard target2 = Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_WEST>(pawn) & board.getoccBB(Seraphina::Color::WHITE) & opts;

				while (ptarget)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(ptarget);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::BLACK_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::BLACK_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::BLACK_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION, Seraphina::PieceType::BLACK_QUEEN));
				}

				while (target)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_WEST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_WEST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_WEST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_WEST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_QUEEN));
				}

				while (target2)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(target2);
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_EAST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_KNIGHT));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_EAST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_BISHOP));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_EAST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_ROOK));
					moves.emplace_back(setMove((Seraphina::Square)(to + Seraphina::Direction::NORTH_EAST), to, Seraphina::PieceType::BLACK_PAWN, Seraphina::MoveType::PROMOTION_CAPTURE, Seraphina::PieceType::BLACK_QUEEN));
				}
			}
		}
	}
	else
	{
		Seraphina::Color pov = board.currPOV();
		Seraphina::PieceType pt = Seraphina::makepiece(pov, p);
		U64 occ = board.getoccBB(Seraphina::Color::NO_COLOR);
		Bitboard movers = board.getPieceBB(pt);

		while (movers)
		{
			Seraphina::Square from = Seraphina::Bitboards::poplsb(movers);
			Bitboard target = Seraphina::Bitboards::getPieceAttacks(from, occ, p) & opts;

			if (mt == Seraphina::MoveType::NORMAL)
			{
				Bitboard targets = target & occ;

				while (targets)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(targets);
					moves.emplace_back(setMove(from, to, pt, mt, board.getBoard(to)));
				}
			}

			if (mt == Seraphina::MoveType::CAPTURE)
			{
				Seraphina::Color xpov = (Seraphina::Color)(pov ^ 1);
				Bitboard targets = target & ~board.getoccBB(xpov);

				while (targets)
				{
					Seraphina::Square to = Seraphina::Bitboards::poplsb(targets);
					moves.emplace_back(setMove(from, to, pt, mt, board.getBoard(to)));
				}
			}
		}
	}
}

void MoveList::generateCastling(Board& board)
{
	if (board.currPOV() == Seraphina::Color::WHITE)
	{
		Seraphina::Square from = Seraphina::Bitboards::lsb(board.getPieceBB(Seraphina::PieceType::WHITE_KING));

		if (board.castling & Seraphina::CastlingType::WKSC)
		{
			if (!board.isChecked(Seraphina::Color::WHITE) &&
				!board.isAttacked(Seraphina::Color::WHITE, Seraphina::Square::SQ_G1))
			{
				moves.emplace_back(setMove(from, (Seraphina::Square)(from + 2), Seraphina::PieceType::WHITE_KING, Seraphina::MoveType::SHORT_CASTLE));
			}
		}

		if (board.castling & Seraphina::CastlingType::WKLC)
		{
			if (!board.isChecked(Seraphina::Color::WHITE) &&
				!board.isAttacked(Seraphina::Color::WHITE, Seraphina::Square::SQ_C1))
			{
				moves.emplace_back(setMove(from, (Seraphina::Square)(from - 2), Seraphina::PieceType::WHITE_KING, Seraphina::MoveType::LONG_CASTLE));
			}
		}
	}
	else
	{
		Seraphina::Square from = Seraphina::Bitboards::lsb(board.getPieceBB(Seraphina::PieceType::BLACK_KING));

		if (board.castling & Seraphina::CastlingType::BKSC)
		{
			if (!board.isChecked(Seraphina::Color::BLACK) &&
				!board.isAttacked(Seraphina::Color::BLACK, Seraphina::Square::SQ_G8))
			{
				moves.emplace_back(setMove(from, (Seraphina::Square)(from + 2), Seraphina::PieceType::BLACK_KING, Seraphina::MoveType::SHORT_CASTLE));
			}
		}

		if (board.castling & Seraphina::CastlingType::BKLC)
		{
			if (!board.isChecked(Seraphina::Color::BLACK) &&
				!board.isAttacked(Seraphina::Color::BLACK, Seraphina::Square::SQ_C8))
			{
				moves.emplace_back(setMove(from, (Seraphina::Square)(from - 2), Seraphina::PieceType::BLACK_KING, Seraphina::MoveType::LONG_CASTLE));
			}
		}
	}
}

template<Seraphina::MoveType mt>
void MoveList::generatePseudoLegal(Board& board)
{
	Bitboard opts = !board.getCheckers() ? -1ULL
		: board.getSQbtw(Seraphina::Bitboards::lsb(board.getPieceBB(board.currPOV() == Seraphina::Color::WHITE
			? Seraphina::PieceType::WHITE_KING : Seraphina::PieceType::BLACK_KING)),
			Seraphina::Bitboards::lsb(board.getCheckers())) | board.getCheckers();

	generatebyPieceType<Seraphina::PieceList::PAWN>(board, mt, opts);
	generatebyPieceType<Seraphina::PieceList::KNIGHT>(board, mt, opts);
	generatebyPieceType<Seraphina::PieceList::BISHOP>(board, mt, opts);
	generatebyPieceType<Seraphina::PieceList::ROOK>(board, mt, opts);
	generatebyPieceType<Seraphina::PieceList::QUEEN>(board, mt, opts);
	generatebyPieceType<Seraphina::PieceList::KING>(board, mt, opts);
	generateCastling(board);
}

void MoveList::generateQuiet(Board& board)
{
	generatePseudoLegal<Seraphina::MoveType::NORMAL>(board);
	generatePseudoLegal<Seraphina::MoveType::SHORT_CASTLE>(board);
	generatePseudoLegal<Seraphina::MoveType::LONG_CASTLE>(board);
}

void MoveList::generateNoisy(Board& board)
{
	generatePseudoLegal<Seraphina::MoveType::CAPTURE>(board);
	generatePseudoLegal<Seraphina::MoveType::ENPASSNT>(board);
	generatePseudoLegal<Seraphina::MoveType::PROMOTION>(board);
	generatePseudoLegal<Seraphina::MoveType::PROMOTION_CAPTURE>(board);
}