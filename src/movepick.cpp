// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "movepick.h"
#include "bitboard.h"
#include "movegen.h"
#include "search.h"

constexpr int PieceValue[13] = { 208, 781, 825, 1276, 2538, 32001, 208, 781, 825, 1276, 2538, 32001, 0 };

namespace Seraphina
{
	template<MovePickType mpt>
	Move MovePicker::select()
	{
		while (current < end)
		{
			if (mpt == MovePickType::Best)
			{
				std::swap(*current, *std::max_element(current, end));
			}

			return *current++;
		}

		return 0;
	}

	Move MovePicker::nextMove(Board& board, bool skipQuiets)
	{
		switch (stage)
		{
		case Stages::TT_MOVE:
			if (isPseudoLegal(board, ttMove))
			{
				return ttMove;
			}
			[[fallthrough]];

		case Stages::GOOD_QUIET:
			while (current < end)
			{
				if (getMoveType(*current) == MoveType::NORMAL)
				{
					return *current++;
				}

				current++;
			}
			[[fallthrough]];

		case Stages::QS_CAPTURE:
			current = endBad;
			movelist->generatePseudoLegal<MoveType::CAPTURE>(board);
			auto moves = movelist->getMoves();
			end = moves.data();
			[[fallthrough]];
		}

		return 0;
	}

	template<MoveType mt>
	void MovePicker::score(Board& board)
	{
		Bitboard threatenedByPawn, threatenedByMinor, threatenedByRook, threatenedPieces;
		Color pov = board.currPOV();
		int value = 0;

		if (mt == MoveType::NORMAL)
		{
			threatenedByPawn = board.getThreatenedBy(PieceList::PAWN);
			threatenedByMinor = threatenedByPawn | board.getThreatenedBy(PieceList::KNIGHT) | board.getThreatenedBy(PieceList::BISHOP);
			threatenedByRook = threatenedByMinor | board.getThreatenedBy(PieceList::ROOK);
			threatenedPieces = (board.getPieceBB(makepiece(pov, PieceList::QUEEN)) & threatenedByRook)
				| (board.getPieceBB(makepiece(pov, PieceList::ROOK)) & threatenedByMinor)
				| ((board.getPieceBB(makepiece(pov, PieceList::KNIGHT)) | board.getPieceBB(makepiece(pov, PieceList::BISHOP))) & threatenedByPawn);
		}

		const Bitboard threats[3] = { threatenedByPawn, threatenedByMinor, threatenedByRook };

		while (current < end)
		{
			const Move& move = *current;
			const Square from = getFrom(move);
			const Square to = getTo(move);
			const PieceType pt = getPieceType(move);
			const PieceList p = getpiece(pt);
			// const int captured = getMoveType(move) == MoveType::ENPASSNT ? PieceList::PAWN : getpiece(board.getPieceType(to));

			if (mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE)
			{
				value = 7 * PieceValue[board.getBoard(to)]
					+ (*captureHistory)[getpiece(board.getBoard(to))][pt][to];
			}

			else if (mt == MoveType::NORMAL)
			{
				value = (*mainHistory)[pov][getFromTo(move)];
				value += ((*pawnHistory)[board.getBoardInfo()->pawnZobrist & (PAWN_HISTORY_SIZE - 1)][pt][to] << 1);


				value += bool(board.getCheckers() ? (threats[std::max(0, p - PieceList::BISHOP)] & to) : (0 & to)) * 16384;
			}

			else
			{
				if (getpiece(getCapPromo(move)) == PieceList::QUEEN)
				{
					value = PieceValue[board.getBoard(to)] - pt + (1 << 28);
				}
				else
				{
					value = (*mainHistory)[pov][getFromTo(move)]
						+ (*continuationHistory[0])[pt][to]
						+ (*pawnHistory)[board.getBoardInfo()->pawnZobrist & (PAWN_HISTORY_SIZE - 1)][pt][to];
				}
			}
		}

		setScore(*current, value);
	}
}