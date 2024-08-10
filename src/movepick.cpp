#include "movepick.h"
#include "bitboard.h"
#include "movegen.h"
#include "search.h"

namespace Seraphina
{
	Move MovePicker::nextMove()
	{
		if (current == end)
		{
			return 0;
		}

		Move m = *current;
		current++;

		return m;
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
			Color pov = board.currPOV();
			// const int captured = getMoveType(move) == MoveType::ENPASSNT ? PieceList::PAWN : getpiece(board.getPieceType(to));

			if (mt == MoveType::CAPTURE || mt == MoveType::PROMOTION_CAPTURE)
			{
				value = 7 * board.getPieceValue(board.getBoard(to))
					+ (*captureHistory)[getpiece((PieceType)board.getBoard(to))][pt][to];
			}

			else if (mt == MoveType::NORMAL)
			{
				value = (*mainHistory)[pov][getFromTo(move)];
				value += ((*pawnHistory)[board.getBoardInfo()->pawnZobrist & (PAWN_HISTORY_SIZE - 1)][pt][to] << 1);
				value += ((*continuationHistory[0])[pt][to] << 1);
				value += (*continuationHistory[1])[pt][to];
				value += (*continuationHistory[2])[pt][to] / 3;
				value += (*continuationHistory[3])[pt][to];
				value += (*continuationHistory[5])[pt][to];

				value += bool(board.isChecked(pov) ? (threats[std::max(0, pt - PieceList::BISHOP)] & to) : (0 & to)) * 16384;

				value += threatenedPieces & from
					? (pt == PieceList::QUEEN && !(to & threatenedByRook) ? 51700
					: pt == PieceList::ROOK && !(to & threatenedByMinor) ? 25600
					: !(to & threatenedByPawn) ? 14450
					: 0)
					: 0;

				value -= (pt == PieceList::QUEEN ? bool(to & threatenedByRook) * 49000
					: pt == PieceList::ROOK ? bool(to & threatenedByMinor) * 24335
					: bool(to & threatenedByPawn) * 14900);
			}

			else
			{
				if (getCaptured(move) || getCapPromo(move) == PieceList::QUEEN)
				{
					value = board.getPieceValue(board.getBoard(to)) - pt + (1 << 28);
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