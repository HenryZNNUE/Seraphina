// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "movegen.h"
#include "bitboard.h"
#include "nnue.h"

namespace Seraphina
{
	void makemove(Board& board, const Move& move, bool update)
	{
		board.addBoardInfo();
		BoardInfo history = *board.getBoardInfo();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		MoveType mt = getMoveType(move);
		PieceType cappromo = getCapPromo(move);
		Color pov = board.get_pov();

		board.setFiftyIncremental();

		if (get_piece(pt) == PieceList::PAWN)
		{
			board.resetFifty();

			if (mt == MoveType::NORMAL)
			{
				board.removePiece(from);
				board.setPiece(to, pt);
			}

			if (mt == MoveType::CAPTURE)
			{
				board.getBoardInfo()->capture = board.getBoard(to);
				board.removePiece(from);
				board.replacePiece(to, pt);

				board.removeCount(cappromo);
				board.removeCountValues(cappromo);
			}

			if (mt == ENPASSNT)
			{
				Direction push = (pov == Color::WHITE ? Direction::NORTH : Direction::SOUTH);

				if ((from ^ to) == 16 && (Bitboards::getPawnAttacks(pov, (to - push)) & board.getPieceBB(make_piece(~pov, PieceList::PAWN))))
				{
					history.ENPASSNT = to - push;

					board.removePiece(from);
					board.replacePiece(history.ENPASSNT, pt);

					board.removeCount(cappromo);
					board.removeCountValues(cappromo);
				}
			}

			if (mt == MoveType::PROMOTION)
			{
				board.removePiece(from);
				board.setPiece(to, cappromo);

				board.removeCount(pt);
				board.addCount(cappromo);
				board.addCountValues(cappromo);
			}

			if (mt == MoveType::PROMOTION_CAPTURE)
			{
				board.getBoardInfo()->capture = board.getBoard(to);
				board.removePiece(from);
				board.replacePiece(to, cappromo);

				board.removeCount(board.getBoardInfo()->capture);
				board.removeCountValues(board.getBoardInfo()->capture);
				board.addCount(cappromo);
				board.addCountValues(cappromo);
			}
		}
		else
		{
			if (mt == MoveType::NORMAL)
			{
				board.removePiece(from);
				board.setPiece(to, pt);
			}

			if (mt == MoveType::SHORT_CASTLE)
			{
				board.castling &= board.castling_rights[from];
				board.castling &= board.castling_rights[to];

				if (pov == Color::WHITE)
				{
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
				board.castling &= board.castling_rights[from];
				board.castling &= board.castling_rights[to];

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

			if (mt == MoveType::CAPTURE)
			{
				board.getBoardInfo()->capture = board.getBoard(to);
				board.removePiece(from);
				board.replacePiece(to, pt);

				board.resetFifty();
			}
		}

		history.repetition = 0;
		int end = std::min(board.getFifty(), board.getpliesFromNull());
		BoardInfo prev = *(board.getHistory().rend() + 2);

		if (end >= 4)
		{
			for (int i = 4; i <= end; i += 2)
			{
				prev = *(board.getHistory().rend() + 4);

				if (history.zobrist == prev.zobrist)
				{
					history.repetition = prev.repetition ? -i : i;
					break;
				}
			}
		}

		board.set_pov((Color)(~pov));
		board.setMoveNumIncremental();
		board.setpliesFromNullIncremental();
		board.setHistoryIncremental(history);
		board.updateZobrist(move);

		board.setCBP();
		board.setThreats();

		if (update)
		{
			board.acc->computed[Color::WHITE] = board.acc->computed[Color::BLACK] = false;
		}
	}

	void undomove(Board& board, const Move& move)
	{
		BoardInfo& history = *board.getBoardInfo();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		MoveType mt = getMoveType(move);
		PieceType cappromo = getCapPromo(move);
		Color pov = board.get_pov();
		int prevPOV = ~pov;
		board.set_pov((Color)prevPOV);
		board.acc--;
		board.undoMoveNumIncremental();
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

				board.setPiece(Square::SQ_E1, PieceType::WHITE_KING);
				board.removePiece(Square::SQ_H1);
			}
			else
			{
				board.setPiece(Square::SQ_H8, PieceType::BLACK_ROOK);
				board.removePiece(Square::SQ_F8);

				board.setPiece(Square::SQ_E8, PieceType::BLACK_KING);
				board.removePiece(Square::SQ_H8);
			}
		}

		if (mt == MoveType::LONG_CASTLE)
		{
			if (prevPOV == Color::WHITE)
			{
				board.setPiece(Square::SQ_A1, PieceType::WHITE_ROOK);
				board.removePiece(Square::SQ_D1);

				board.setPiece(Square::SQ_E1, PieceType::WHITE_KING);
				board.removePiece(Square::SQ_C1);
			}
			else
			{
				board.setPiece(Square::SQ_A8, PieceType::BLACK_ROOK);
				board.removePiece(Square::SQ_D8);

				board.setPiece(Square::SQ_E8, PieceType::BLACK_KING);
				board.removePiece(Square::SQ_C8);
			}
		}

		if (mt == MoveType::CAPTURE)
		{
			board.replacePiece(to, cappromo);
			board.setPiece(from, pt);

			board.addCount(cappromo);
			
			if (get_piece(cappromo) != PieceList::PAWN)
			{
				board.addCountValues(cappromo);
			}
		}

		if (mt == MoveType::ENPASSNT)
		{
			board.replacePiece(history.ENPASSNT, make_piece(pov, PieceList::PAWN));
			board.setPiece(from, pt);

			board.addCount(cappromo);
		}

		if (mt == MoveType::PROMOTION)
		{
			board.removePiece(to);
			board.setPiece(from, pt);

			board.removeCount(cappromo);
			board.removeCountValues(cappromo);
			board.addCount(pt);
		}

		if (mt == MoveType::PROMOTION_CAPTURE)
		{
			board.replacePiece(to, cappromo);
			board.setPiece(from, pt);

			board.removeCount(cappromo);
			board.removeCountValues(cappromo);
			board.addCount(pt);

			board.addCount(history.capture);
			board.addCountValues(history.capture);
		}
	}

	std::string movetoString(Board& board, const Move& move)
	{
		Square from = getFrom(move);
		Square to = getTo(move);
		MoveType mt = getMoveType(move);

		if (move == 0)
		{
			return "0000";
		}

		if ((mt == MoveType::SHORT_CASTLE || mt == MoveType::LONG_CASTLE)
			&& board.isChess960() == false)
		{
			to = make_square(to > from ? File::FILE_G : File::FILE_C, get_file(from));
		}

		std::string str = Seraphina::Bitboards::squaretostr(from) + Seraphina::Bitboards::squaretostr(to);

		if (mt == MoveType::PROMOTION || mt == MoveType::PROMOTION_CAPTURE)
		{
			str += (char)tolower(piece_to_char(getCapPromo(move)));
		}

		return str;
	}
	
	bool isPseudoLegal(Board& board, const Move& move)
	{
		Color pov = board.get_pov();
		int xpov = ~pov;
		int push = board.where_to_push();
		Square from = getFrom(move);
		Square to = getTo(move);
		PieceType pt = getPieceType(move);
		PieceList p = get_piece(pt);
		MoveType mt = getMoveType(move);
		Bitboard checkers = board.getCheckers();

		if (pt == PieceType::NO_PIECETYPE || get_color(pt) == xpov
			|| from == Square::NO_SQ || to == Square::NO_SQ)
		{
			return false;
		}

		if (board.getoccBB(pov) & to)
		{
			return false;
		}

		if (mt == MoveType::SHORT_CASTLE || mt == MoveType::LONG_CASTLE)
		{
			auto castling = (pov == Color::WHITE) ? mt : (mt << 2);
			int castling_rook_sq = board.getCastlingRookSQ(castling);
			Square rook = (pov == Color::WHITE)
				? ((mt == MoveType::SHORT_CASTLE) ? Square::SQ_F1 : Square::SQ_D1)
				: ((mt == MoveType::SHORT_CASTLE) ? Square::SQ_F8 : Square::SQ_D8);
			Bitboard kc = board.getSQbtw(from, to) | Bitboards::bit(to);
			Bitboard rc = board.getSQbtw(castling_rook_sq, rook);
			Bitboard btw = kc | rc;

			if (((board.getoccBB(Color::NO_COLOR)
				^ Bitboards::bit(from)
				^ castling_rook_sq) & btw)
				|| kc & board.getThreatened())
			{
				return false;
			}
		}

		if (mt == MoveType::PROMOTION || mt == MoveType::PROMOTION_CAPTURE)
		{
			if (Bitboards::more_than_one(checkers))
			{
				return false;
			}

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
				if (!(Bitboards::getPawnAttacks(pov, from) & board.getoccBB(xpov) & to)
					&& !((from + push == to)
						&& board.getBoard(to) == PieceType::NO_PIECETYPE)
					&& !((from + (push << 1) == to)
						&& ((from ^ (pov * 7)) == Rank::RANK_2)
						&& board.getBoard(to) == PieceType::NO_PIECETYPE
						&& board.getBoard(to - push) == PieceType::NO_PIECETYPE))
				{
					return false;
				}
			}
			else
			{
				if (to != board.getBoardInfo()->ENPASSNT)
				{
					return false;
				}
			}
		}
		else if (!(Bitboards::getPieceAttacks(from, board.getoccBB(Color::NO_COLOR), p) & to))
		{
			return false;
		}

		if (mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE)
		{
			if (board.getBoardInfo()->ENPASSNT != Square::NO_SQ)
			{
				if (to != board.getBoardInfo()->ENPASSNT)
				{
					return false;
				}
			}

			if (board.getBoard(to) == PieceType::NO_PIECETYPE)
			{
				return false;
			}
		}

		if (mt != MoveType::CAPTURE && board.getBoard(to) != PieceType::NO_PIECETYPE)
		{
			return false;
		}

		if (checkers)
		{
			if (p != PieceList::KING)
			{
				if (Bitboards::more_than_one(checkers))
				{
					return false;
				}

				if (!(board.getSQbtw(board.getKingSquare(pov), Bitboards::lsb(checkers)) & to))
				{
					return false;
				}
			}
			else if (!board.isAttacked(to, board.getoccBB(Color::NO_COLOR), xpov))
			{
				return false;
			}
		}

		return true;
	}
}

void MoveList::generatePawn(Board& board, Seraphina::MoveType mt, Bitboard& target, Bitboard& checkers)
{
	Seraphina::Color pov = board.get_pov();
	int xpov = ~pov;
	Seraphina::PieceType pt = Seraphina::make_piece(pov, Seraphina::PieceList::PAWN);
	Seraphina::Direction push = (pov == Seraphina::Color::WHITE ? Seraphina::Direction::NORTH : Seraphina::Direction::SOUTH);
	Seraphina::Direction push2 = (Seraphina::Direction)(pov == Seraphina::Color::WHITE ? (Seraphina::Direction::NORTH << 1) : (Seraphina::Direction::SOUTH * 2));
	Seraphina::Direction left = (pov == Seraphina::Color::WHITE ? Seraphina::Direction::NORTH_WEST : Seraphina::Direction::SOUTH_EAST);
	Seraphina::Direction right = (pov == Seraphina::Color::WHITE ? Seraphina::Direction::NORTH_EAST : Seraphina::Direction::SOUTH_WEST);

	Bitboard TR7 = (pov == Seraphina::Color::WHITE ? Rank7BB : Rank2BB);
	Bitboard TR3 = (pov == Seraphina::Color::WHITE ? Rank3BB : Rank6BB);

	Bitboard pawn = board.getPieceBB(pt) & ~TR7;
	Bitboard ppawn = board.getPieceBB(pt) & TR7;

	Bitboard opts = checkers ? board.getCheckers() : board.getoccBB(xpov);

	if (mt == Seraphina::MoveType::NORMAL)
	{
		Bitboard p1 = Seraphina::Bitboards::shift(push, pawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR);
		Bitboard p2 = Seraphina::Bitboards::shift(push, p1 & TR3) & ~board.getoccBB(Seraphina::Color::NO_COLOR);

		if (checkers)
		{
			p1 &= target;
			p2 &= target;
		}

		while (p1)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(p1);
			moves.emplace_back(setMove((to - push), to, pt, mt));
		}

		while (p2)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(p2);
			moves.emplace_back(setMove((to - push2), to, pt, mt));
		}
	}

	else if (mt == Seraphina::MoveType::CAPTURE)
	{
		Bitboard c1 = Seraphina::Bitboards::shift(right, pawn) & opts;
		Bitboard c2 = Seraphina::Bitboards::shift(left, pawn) & opts;

		while (c1)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(c1);
			moves.emplace_back(setMove((to - right), to, pt, mt, board.getBoard(to)));
		}

		while (c2)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(c2);
			moves.emplace_back(setMove((to - left), to, pt, mt, board.getBoard(to)));
		}

		if (board.getBoardInfo()->ENPASSNT != Seraphina::Square::NO_SQ)
		{
			int ep = board.getBoardInfo()->ENPASSNT;

			if (!checkers && !(target & (ep + push)))
			{
				Bitboard e = pawn & Seraphina::Bitboards::getPawnAttacks(xpov, ep);

				while (e)
				{
					moves.emplace_back(setMove(Seraphina::Bitboards::poplsb(e), ep, pt, Seraphina::MoveType::ENPASSNT, board.getBoard(ep)));
				}
			}
		}
	}

	else
	{
		Bitboard promo = Seraphina::Bitboards::shift(push, ppawn) & ~board.getoccBB(Seraphina::Color::NO_COLOR);
		Bitboard cp1 = Seraphina::Bitboards::shift(right, ppawn) & opts;
		Bitboard cp2 = Seraphina::Bitboards::shift(left, ppawn) & opts;

		Seraphina::PieceType pn = Seraphina::make_piece(pov, Seraphina::PieceList::KNIGHT);
		Seraphina::PieceType pb = Seraphina::make_piece(pov, Seraphina::PieceList::BISHOP);
		Seraphina::PieceType pr = Seraphina::make_piece(pov, Seraphina::PieceList::ROOK);
		Seraphina::PieceType pq = Seraphina::make_piece(pov, Seraphina::PieceList::QUEEN);

		if (checkers)
		{
			promo &= target;
		}

		while (promo)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(promo);
			int from = (to - push);

			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION, pn));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION, pb));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION, pr));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION, pq));
		}

		while (cp1)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(cp1);
			int from = (to - right);

			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pn));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pb));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pr));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pq));
		}

		while (cp2)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(cp2);
			int from = (to - left);

			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pn));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pb));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pr));
			moves.emplace_back(setMove(from, to, pt, Seraphina::MoveType::PROMOTION_CAPTURE, pq));
		}
	}
}

void MoveList::generateKing(Board& board, Seraphina::MoveType mt, Bitboard& target, Bitboard& checkers)
{
	Seraphina::Color pov = board.get_pov();
	Seraphina::PieceType pt = Seraphina::make_piece(pov, Seraphina::PieceList::KING);
	Seraphina::Square KingSq = board.getKingSquare(pov);
	Bitboard b = Seraphina::Bitboards::getKingAttacks(board.getKingSquare(pov))
		& (checkers ? ~board.getoccBB(pov) : target);

	while (b)
	{
		Seraphina::Square to = Seraphina::Bitboards::poplsb(b);

		if (mt == Seraphina::MoveType::NORMAL)
		{
			moves.emplace_back(setMove(KingSq, to, pt, mt));
		}

		if (mt == Seraphina::MoveType::CAPTURE)
		{
			moves.emplace_back(setMove(KingSq, to, pt, mt, board.getBoard(to)));
		}
	}

	if ((mt == Seraphina::MoveType::NORMAL && !checkers)
		&& board.getCastlingRights(pov & Seraphina::CastlingType::ALL_CASTLING))
	{
		if (pov == Seraphina::Color::WHITE)
		{
			if (board.getCastlingRights(Seraphina::CastlingType::WKSC))
			{
				Bitboard ksc = board.getSQbtw(KingSq, Seraphina::Square::SQ_G1);
				Bitboard rsc = board.getSQbtw(Seraphina::Square::SQ_H1, Seraphina::Square::SQ_F1);
				Bitboard btw = ksc | rsc;

				if (!((board.getoccBB(Seraphina::Color::NO_COLOR)
					^ Seraphina::Bitboards::bit(KingSq)
					^ Seraphina::Bitboards::bit(Seraphina::Square::SQ_H1))
					& btw))
				{
					if (!(board.getThreatened() & ksc))
					{
						moves.emplace_back(setMove(Seraphina::Square::SQ_E1, Seraphina::Square::SQ_G1, pt, Seraphina::MoveType::SHORT_CASTLE));
					}
				}
			}

			if (board.getCastlingRights(Seraphina::CastlingType::WKLC))
			{
				Bitboard klc = board.getSQbtw(KingSq, Seraphina::Square::SQ_C1);
				Bitboard rlc = board.getSQbtw(Seraphina::Square::SQ_A1, Seraphina::Square::SQ_D1);
				Bitboard btw = klc | rlc;

				if (!((board.getoccBB(Seraphina::Color::NO_COLOR)
					^ Seraphina::Bitboards::bit(KingSq)
					^ Seraphina::Bitboards::bit(Seraphina::Square::SQ_A1))
					& btw))
				{
					if (!(board.getThreatened() & klc))
					{
						moves.emplace_back(setMove(Seraphina::Square::SQ_E1, Seraphina::Square::SQ_C1, pt, Seraphina::MoveType::LONG_CASTLE));
					}
				}
			}
		}
		else
		{
			if (board.getCastlingRights(Seraphina::CastlingType::BKSC))
			{
				Bitboard ksc = board.getSQbtw(KingSq, Seraphina::Square::SQ_G8);
				Bitboard rsc = board.getSQbtw(Seraphina::Square::SQ_H8, Seraphina::Square::SQ_F8);
				Bitboard btw = ksc | rsc;

				if (!((board.getoccBB(Seraphina::Color::NO_COLOR)
					^ Seraphina::Bitboards::bit(KingSq)
					^ Seraphina::Bitboards::bit(Seraphina::Square::SQ_H8))
					& btw))
				{
					if (!(board.getThreatened() & ksc))
					{
						moves.emplace_back(setMove(Seraphina::Square::SQ_E8, Seraphina::Square::SQ_G8, pt, Seraphina::MoveType::SHORT_CASTLE));
					}
				}
			}

			if (board.getCastlingRights(Seraphina::CastlingType::BKLC))
			{
				Bitboard klc = board.getSQbtw(KingSq, Seraphina::Square::SQ_C8);
				Bitboard rlc = board.getSQbtw(Seraphina::Square::SQ_A8, Seraphina::Square::SQ_D8);
				Bitboard btw = klc | rlc;

				if (!((board.getoccBB(Seraphina::Color::NO_COLOR)
					^ Seraphina::Bitboards::bit(KingSq)
					^ Seraphina::Bitboards::bit(Seraphina::Square::SQ_A8))
					& btw))
				{
					if (!(board.getThreatened() & klc))
					{
						moves.emplace_back(setMove(Seraphina::Square::SQ_E8, Seraphina::Square::SQ_C8, pt, Seraphina::MoveType::LONG_CASTLE));
					}
				}
			}
		}
	}
}

template <Seraphina::PieceList p>
void MoveList::generatebyPieceType(Board& board, Seraphina::MoveType mt, Bitboard& target)
{
	Seraphina::Color pov = board.get_pov();
	Seraphina::PieceType pt = Seraphina::make_piece(pov, p);
	U64 occ = board.getoccBB(Seraphina::Color::NO_COLOR);
	Bitboard bb = board.getPieceBB(pt);

	while (bb)
	{
		Seraphina::Square from = Seraphina::Bitboards::poplsb(bb);
		Bitboard b = Seraphina::Bitboards::getPieceAttacks(from, occ, p) & target;

		while (b)
		{
			Seraphina::Square to = Seraphina::Bitboards::poplsb(b);

			if (mt == Seraphina::MoveType::NORMAL)
			{
				moves.emplace_back(setMove(from, to, pt, mt));
			}

			if (mt == Seraphina::MoveType::CAPTURE)
			{
				moves.emplace_back(setMove(from, to, pt, mt, board.getBoard(to)));
			}
		}
	}
}

template<Seraphina::MoveType mt>
void MoveList::generatePseudoLegal(Board& board)
{
	Seraphina::Color pov = board.get_pov();
	int xpov = ~pov;
	Bitboard checkers = board.getCheckers();
	Bitboard target;

	if (!checkers || !Seraphina::Bitboards::more_than_one(checkers))
	{
		target = checkers ? board.getSQbtw(board.getKingSquare(pov), Seraphina::Bitboards::lsb(checkers))
			: !checkers ? ~board.getoccBB(pov)
			: mt == Seraphina::MoveType::CAPTURE || mt == Seraphina::MoveType::PROMOTION_CAPTURE ? board.getoccBB(xpov)
			: ~board.getoccBB(Seraphina::Color::NO_COLOR);

		generatePawn(board, mt, target, checkers);
		generatebyPieceType<Seraphina::PieceList::KNIGHT>(board, mt, target);
		generatebyPieceType<Seraphina::PieceList::BISHOP>(board, mt, target);
		generatebyPieceType<Seraphina::PieceList::ROOK>(board, mt, target);
		generatebyPieceType<Seraphina::PieceList::QUEEN>(board, mt, target);
	}

	generateKing(board, mt, target, checkers);
}

void MoveList::generateQuiet(Board& board)
{
	generatePseudoLegal<Seraphina::MoveType::NORMAL>(board);
}

void MoveList::generateNoisy(Board& board)
{
	generatePseudoLegal<Seraphina::MoveType::CAPTURE>(board);
	generatePseudoLegal<Seraphina::MoveType::PROMOTION_CAPTURE>(board);
}

void MoveList::generateAll(Board& board)
{
	generatePseudoLegal<Seraphina::MoveType::NORMAL>(board);
	generatePseudoLegal<Seraphina::MoveType::CAPTURE>(board);
	generatePseudoLegal<Seraphina::MoveType::PROMOTION>(board);
	generatePseudoLegal<Seraphina::MoveType::PROMOTION_CAPTURE>(board);
}