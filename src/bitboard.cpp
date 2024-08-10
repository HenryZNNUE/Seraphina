// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include <sstream>

#include "bitboard.h"
#include "movegen.h"
#include "nnue.h"
#include "tt.h"

// Square String
const char* SquareCoor[SQ_NUM] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

constexpr int PieceValue[13] = { 208, 781, 825, 1276, 2538, 32001, 208, 781, 825, 1276, 2538, 32001, 0 };

namespace Seraphina
{
	namespace Bitboards
	{
        Bitboard getPawnAttacks(Color pov, Square sq)
        {
            return PAWN_ATTACKS_TABLE[pov][sq];
        }

        Bitboard getKnightAttacks(Square sq)
        {
            return KNIGHT_ATTACKS_TABLE[sq];
        }

        Bitboard getBishopAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::BishopAttacks(sq, occ);
        }

        Bitboard getRookAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::RookAttacks(sq, occ);
        }

        Bitboard getQueenAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::QueenAttacks(sq, occ);
        }

        Bitboard getKingAttacks(Square sq)
        {
            return KING_ATTACKS_TABLE[sq];
        }

        Bitboard getPieceAttacks(Square sq, Bitboard occ, PieceList p)
        {
            switch (p)
            {
                case PieceList::KNIGHT:
                    return getKnightAttacks(sq);

                case PieceList::BISHOP:
                    return getBishopAttacks(sq, occ);

                case PieceList::ROOK:
					return getRookAttacks(sq, occ);

                case PieceList::QUEEN:
					return getQueenAttacks(sq, occ);

                case PieceList::KING:
                    return getKingAttacks(sq);

                default:
                    return 0ULL;
            }
        }

        // Print Bitboard function which is a bit similar to Stockfish's
        // Use const reference can avoid copying when printing Bitboard which improves performance
        void printBB(const Bitboard& bb)
        {
            std::cout << "+---+---+---+---+---+---+---+---+\n";

            for (int r = Rank::RANK_8; r >= Rank::RANK_1; r--)
            {
                for (int f = File::FILE_A; f <= File::FILE_H; f++)
                {
                    ((1ULL << 0) & bb) ? std::cout << "| X " : std::cout << "|   ";
                }

                std::cout << "|" << r << "\n";
                std::cout << "+---+---+---+---+---+---+---+---+\n";
            }

            std::cout << "  a   b   c   d   e   f   g   h\n";
        }
	}
}

U64 Board::randU64()
{
    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

// Initialize Zobrist Hashing by filling the Zobrist array with random numbers
void Board::initZobrist()
{
    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; pt++)
    {
        for (int sq = Seraphina::Square::SQ_A1; sq < Seraphina::Square::NO_SQ; sq++)
        {
			Zobrist[pt][sq] = randU64();
		}
    }

    for (int file = Seraphina::File::FILE_A; file < Seraphina::File::NO_FILE; file++)
    {
		ZobristEP[file] = randU64();
    }

    for (int i = 0; i < 16; i++)
    {
        ZobristCastle[i] = randU64();
    }

    ZobristSide = randU64();
}

void Board::generatePawnZobrist()
{
    U64 hash = 0ULL;

    Bitboard wp = pieceBB[Seraphina::PieceType::WHITE_PAWN];

    while (wp)
    {
        hash ^= Zobrist[Seraphina::PieceType::WHITE_PAWN][Seraphina::Bitboards::poplsb(wp)];
    }

    Bitboard bp = pieceBB[Seraphina::PieceType::BLACK_PAWN];

    while (bp)
    {
        hash ^= Zobrist[Seraphina::PieceType::BLACK_PAWN][Seraphina::Bitboards::poplsb(bp)];
    }

    if (currentPlayer)
    {
        hash ^= ZobristSide;
    }

    getBoardInfo()->pawnZobrist = hash;
}

// Generate Zobrist Hashing
void Board::generateZobrist()
{
	U64 hash = 0ULL;

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; pt++)
    {
        Bitboard pieces = pieceBB[(Seraphina::PieceType)pt];

        while (pieces)
        {
            hash ^= Zobrist[pt][Seraphina::Bitboards::poplsb(pieces)];
        }
	}

    if (getBoardInfo()->ENPASSNT)
    {
		hash ^= ZobristEP[getBoardInfo()->ENPASSNT];
	}

	hash ^= ZobristCastle[castling];

    if (currentPlayer)
    {
        hash ^= ZobristSide;
    }

	getBoardInfo()->zobrist = hash;
}

void Board::updateZobrist(const Move& move)
{
	const Seraphina::Square from = Seraphina::getFrom(move);
	const Seraphina::Square to = Seraphina::getTo(move);
	const Seraphina::PieceType pt = Seraphina::getPieceType(move);
    const Seraphina::MoveType mt = Seraphina::getMoveType(move);

    if (!move)
    {
        getBoardInfo()->zobrist ^= ZobristSide;
        return;
    }

    U64 updated = 0ULL;
	updated = getBoardInfo()->zobrist ^ ZobristSide ^ Zobrist[pt][from] ^ Zobrist[pt][to];

    if (board[to] != Seraphina::PieceType::NO_PIECETYPE)
    {
        updated ^= Zobrist[board[to]][to];
    }

    if (mt == Seraphina::MoveType::ENPASSNT)
    {
        updated ^= ZobristEP[getBoardInfo()->ENPASSNT];
    }
    
    if (mt == Seraphina::MoveType::SHORT_CASTLE || mt == Seraphina::MoveType::LONG_CASTLE)
    {
        updated ^= ZobristCastle[castling];
    }

    if (Seraphina::getpiece(pt) == Seraphina::PieceList::PAWN)
    {
        if (mt == Seraphina::MoveType::CAPTURE || mt == Seraphina::MoveType::PROMOTION_CAPTURE)
        {
            getBoardInfo()->pawnZobrist ^= Zobrist[getBoardInfo()->capture][to];
        }

        else if (mt == Seraphina::MoveType::ENPASSNT)
        {
            int ENPASSNT = to - (currentPlayer == Seraphina::Color::WHITE
                ? Seraphina::Direction::NORTH : Seraphina::Direction::SOUTH);
            getBoardInfo()->pawnZobrist ^= Zobrist[getBoardInfo()->capture][ENPASSNT];
        }

        else
        {
            getBoardInfo()->pawnZobrist ^= Zobrist[pt][from] ^ Zobrist[pt][to];
        }
    }

    getBoardInfo()->zobrist = updated;
}

void Board::initCuckoo()
{
    int count = 0;
    cuckoo.fill(0);
    cuckooMove.fill(0);

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt <= Seraphina::PieceType::BLACK_KING; pt++)
	{
		for(int sq = 0; sq < 64; ++sq)
		{
            for (int sq2 = sq + 1; sq2 < 64; ++sq2)
            {
                if (Seraphina::getpiece((Seraphina::PieceType)pt) != Seraphina::PieceList::PAWN
                    && (Seraphina::Bitboards::getPieceAttacks((Seraphina::Square)sq, 0, Seraphina::getpiece((Seraphina::PieceType)pt)) & Seraphina::Bitboards::bit(sq2)))
                {
                    Move move = Seraphina::setMove((Seraphina::Square)sq, (Seraphina::Square)sq2, (Seraphina::PieceType)pt, Seraphina::MoveType::NORMAL);

                    U64 key = Zobrist[pt][sq] ^ Zobrist[pt][sq2] ^ ZobristSide;
                    int i = H1(key);

                    while (true)
                    {
                        std::swap(cuckoo[i], key);
                        std::swap(cuckooMove[i], move);

                        if (move == 0)
                        {
                            break;
                        }

                        i = (i == H1(key)) ? H2(key) : H1(key);
                    }

                    count++;
                }
            }
		}
	}

    assert(count == 3668);
}

// Inspired by Berserk
void Board::initSQbtw()
{
    int i = 0;

    for (int sq = 0; sq < 64; sq++)
    {
        for (int sq2 = sq + 1; sq2 < 64; sq2++)
        {
            if (Seraphina::getrank((Seraphina::Square)sq) == Seraphina::getrank((Seraphina::Square)sq2))
            {
                i = sq2 + Seraphina::Direction::WEST;

                while (sq2 > sq)
                {
                    SQbtw[sq][sq2] |= Seraphina::Bitboards::bit(i);
                    i += Seraphina::Direction::WEST;
                }
            }
            else if (Seraphina::getfile((Seraphina::Square)sq) == Seraphina::getfile((Seraphina::Square)sq2))
			{
				i = sq2 + Seraphina::Direction::NORTH;

				while (sq2 > sq)
				{
					SQbtw[sq][sq2] |= Seraphina::Bitboards::bit(i);
					i += Seraphina::Direction::NORTH;
				}
			}
			else if ((sq2 - sq) % Seraphina::Direction::NORTH_EAST == 0
                && (Seraphina::getfile((Seraphina::Square)sq) < Seraphina::getfile((Seraphina::Square)sq2)))
			{
				i = sq2 + Seraphina::Direction::NORTH_WEST;

				while (sq2 > sq)
				{
					SQbtw[sq][sq2] |= Seraphina::Bitboards::bit(i);
					i += Seraphina::Direction::NORTH_WEST;
				}
			}
			else if ((sq2 - sq) % Seraphina::Direction::NORTH_WEST == 0
                && (Seraphina::getfile((Seraphina::Square)sq) > Seraphina::getfile((Seraphina::Square)sq2)))
			{
				i = sq2 + Seraphina::Direction::NORTH_EAST;

				while (sq2 > sq)
				{
					SQbtw[sq][sq2] |= Seraphina::Bitboards::bit(i);
					i += Seraphina::Direction::NORTH_EAST;
				}
			}
        }
    }

    for (int sq = 0; sq < 64; sq++)
    {
        for (int sq2 = 0; sq2 < sq; sq2++)
        {
            SQbtw[sq][sq2] = SQbtw[sq2][sq];
        }
    }
}


Seraphina::PieceType Board::getPieceType(Seraphina::Square sq) const
{
	return static_cast<Seraphina::PieceType>(board[sq]);
}

void Board::setPiece(Seraphina::Square sq, Seraphina::PieceType pt)
{
    board[sq] = pt;

    Seraphina::Bitboards::setBit(pieceBB[pt], sq);
    Seraphina::Bitboards::setBit(occBB[pt / 6], sq);
    occBB[Seraphina::Color::NO_COLOR] = occBB[Seraphina::Color::WHITE] | occBB[Seraphina::Color::BLACK];
}

void Board::removePiece(Seraphina::Square sq)
{
    const Seraphina::PieceType pt = getPieceType(sq);

    board[sq] = Seraphina::PieceType::NO_PIECETYPE;
    Seraphina::Bitboards::popBit(pieceBB[pt], sq);
    Seraphina::Bitboards::popBit(occBB[pt / 6], sq);
    occBB[Seraphina::Color::NO_COLOR] = occBB[Seraphina::Color::WHITE] | occBB[Seraphina::Color::BLACK];
}

void Board::replacePiece(Seraphina::Square sq, Seraphina::PieceType pt)
{
	removePiece(sq);
	setPiece(sq, pt);
}

// Set checkers, pinners and king square
void Board::setCPK()
{
	KingSQ = getKingSquare(currentPlayer);
    /*
	checkers = (Seraphina::Bitboards::getKnightAttacks((Seraphina::Square)KingSQ)
        & pieceBB[currPOV() == Seraphina::Color::WHITE
        ? Seraphina::PieceType::BLACK_KNIGHT
        : Seraphina::PieceType::WHITE_KNIGHT]) |
        (Seraphina::Bitboards::getPawnAttacks((Seraphina::Color)currPOV(), (Seraphina::Square)KingSQ)
        & pieceBB[currPOV() == Seraphina::Color::WHITE
        ? Seraphina::PieceType::BLACK_PAWN
        : Seraphina::PieceType::WHITE_PAWN]);
    */
    checkers = attackersTo((Seraphina::Square)KingSQ, occBB[Seraphina::Color::NO_COLOR]);

	Bitboard sliders =
        ((pieceBB[Seraphina::makepiece(currentPlayer, Seraphina::PieceList::BISHOP)] |
        pieceBB[Seraphina::makepiece(currentPlayer, Seraphina::PieceList::QUEEN)])
            & Seraphina::Bitboards::getBishopAttacks((Seraphina::Square)KingSQ, 0)) |
        ((pieceBB[Seraphina::makepiece(currentPlayer, Seraphina::PieceList::ROOK)] |
        pieceBB[Seraphina::makepiece(currentPlayer, Seraphina::PieceList::QUEEN)])
            & Seraphina::Bitboards::getRookAttacks((Seraphina::Square)KingSQ, 0));

    while (sliders)
    {
		int sq = Seraphina::Bitboards::poplsb(sliders);
        Bitboard blockers = SQbtw[(Seraphina::Square)KingSQ][(Seraphina::Square)sq] & occBB[Seraphina::Color::NO_COLOR];

        if (!blockers)
        {
			Seraphina::Bitboards::setBit(checkers, (Seraphina::Square)sq);
        }
		else if (Seraphina::Bitboards::popcount(blockers) == 1)
		{
			pinned |= (blockers & getoccBB((Seraphina::Color)currPOV()));
		}
    }
}

void Board::setThreats()
{
    int other = currentPlayer ^ 1;

    Bitboard occ  = occBB[Seraphina::Color::NO_COLOR]
        ^ pieceBB[Seraphina::makepiece(currentPlayer, Seraphina::PieceList::KING)];

    Bitboard pawnAttacks = currentPlayer ?
        Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_WEST>(pieceBB[Seraphina::PieceType::BLACK_PAWN])
        | Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_EAST>(pieceBB[Seraphina::PieceType::BLACK_PAWN])
        : Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_WEST>(pieceBB[Seraphina::PieceType::WHITE_PAWN])
        | Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_EAST>(pieceBB[Seraphina::PieceType::WHITE_PAWN]);

    threatenedBy[Seraphina::PieceList::PAWN] = pawnAttacks;
    threatened = threatenedBy[Seraphina::PieceList::PAWN];

    for (int p = 0; p < 5; p++)
    {
        Bitboard pieces = pieceBB[Seraphina::makepiece(
            currentPlayer ? Seraphina::Color::WHITE : Seraphina::Color::BLACK,
            (Seraphina::PieceList)p)];

        while (pieces)
        {
            const Seraphina::Square sq = Seraphina::Bitboards::poplsb(pieces);
            Bitboard attacks = 0ULL;

            switch (p)
            {
            case Seraphina::PieceList::KNIGHT:
                threatenedBy[Seraphina::PieceList::KNIGHT] |= Seraphina::Bitboards::getKnightAttacks(sq);
                break;

            case Seraphina::PieceList::BISHOP:
                threatenedBy[Seraphina::PieceList::BISHOP] |= Seraphina::Bitboards::getBishopAttacks(sq, occ);
                break;

            case Seraphina::PieceList::ROOK:
                threatenedBy[Seraphina::PieceList::ROOK] |= Seraphina::Bitboards::getRookAttacks(sq, occ);
                break;

            case Seraphina::PieceList::QUEEN:
                threatenedBy[Seraphina::PieceList::QUEEN] |= Seraphina::Bitboards::getQueenAttacks(sq, occ);
                break;

            default:
                break;
            }
        }
    }

    threatened |= threatenedBy[Seraphina::PieceList::KNIGHT];
    threatened |= threatenedBy[Seraphina::PieceList::BISHOP];
    threatened |= threatenedBy[Seraphina::PieceList::ROOK];
    threatened |= threatenedBy[Seraphina::PieceList::QUEEN];

    threatenedBy[Seraphina::PieceList::KING] |= Seraphina::Bitboards::getKingAttacks(
        Seraphina::Bitboards::lsb(pieceBB[Seraphina::makepiece((Seraphina::Color)other,Seraphina::PieceList::KING)]));
    threatened |= threatenedBy[Seraphina::PieceList::KING];
}

int Board::getPieceValue(Seraphina::PieceType pt) const
{
    return PieceValue[pt];
}

int Board::countPiece(Seraphina::PieceType pt)
{
    for (int sq = 0; sq < SQ_NUM; sq++)
    {
        if (getPieceType((Seraphina::Square)sq) != Seraphina::PieceType::NO_PIECETYPE
            && pieceCount[pt] == getPieceType((Seraphina::Square)sq))
        {
            pieceCount[pt]++;
        }
	}

	return pieceCount[pt];
}

int Board::countPieceValues(Seraphina::Color pov)
{
    for (int pt = 0; pt < 12; pt++)
    {
        if (pt != Seraphina::makepiece(pov, Seraphina::PieceList::PAWN)
            && pt != Seraphina::makepiece(pov, Seraphina::PieceList::KING))
        {
            if (countPiece((Seraphina::PieceType)pt) != 0)
            {
                nonPawns[pov] += PieceValue[pt] * countPiece((Seraphina::PieceType)pt);
            }
        }
    }

	return nonPawns[pov];
}

void Board::parseFEN(char* fen)
{
    ClearBoard();

    for (int sq = 0; sq < 64; sq++)
    {
        if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
        {
            Seraphina::PieceType pt = Seraphina::chartopiece(*fen);
            setPiece((Seraphina::Square)sq, pt);
        }
        else if (*fen >= '0' && *fen <= '9')
        {
            sq += (*fen - '1');
        }
        else if (*fen == '/')
        {
            sq--;
        }

        fen++;
    }

    fen++;

    *fen++ == 'w' ? setPOV(Seraphina::Color::WHITE) : setPOV(Seraphina::Color::BLACK);

    while (*(++fen) != ' ')
    {
        switch (*fen)
        {
		case 'K':
            castling |= Seraphina::CastlingType::WKSC;
			break;

		case 'Q':
			castling |= Seraphina::CastlingType::WKLC;
			break;

		case 'k':
			castling |= Seraphina::CastlingType::BKSC;
			break;

		case 'q':
			castling |= Seraphina::CastlingType::BKLC;
			break;

		default:
			break;
		}
	}
    
	fen++;

    if (*fen != '-')
    {
        getBoardInfo()->ENPASSNT = Seraphina::makesquare((Seraphina::File)(fen[0] - 'a'), (Seraphina::Rank)(8 - (fen[1] - '0')));
	}
    else
    {
        getBoardInfo()->ENPASSNT = 0;
    }

    fen++;

    getBoardInfo()->fifty = atoi(fen);

    fen++;

    movenum = atoi(fen);

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; pt++)
    {
		occBB[pt & 1] |= pieceBB[pt];
	}

    occBB[Seraphina::Color::NO_COLOR] = occBB[Seraphina::Color::WHITE] | occBB[Seraphina::Color::BLACK];

    setCPK();
    setThreats();
    generatePawnZobrist();
	generateZobrist();
}

void Board::BoardtoFEN(char* fen)
{

    for (int rank = Seraphina::Rank::RANK_1; rank <= Seraphina::Rank::RANK_8; rank++)
    {
        int count = 0;

        for (int file = Seraphina::File::FILE_A; file <= Seraphina::File::FILE_H; file++)
        {
            int pt = getPieceType(Seraphina::makesquare((Seraphina::File)file, (Seraphina::Rank)rank));

            if (pt != Seraphina::PieceType::NO_PIECETYPE)
            {
                if (count)
                {
                    *fen++ = count + '0';
                }

                *fen++ = Seraphina::piecetochar((Seraphina::PieceType)pt);
                count = 0;
            }
            else
            {
                count++;
            }
        }

        if (count)
        {
            *fen++ = count + '0';
        }

        *fen++ = (rank == 7) ? ' ' : '/';
    }

    *fen++ = (currPOV() == Seraphina::Color::WHITE) ? 'w' : 'b';
    *fen++ = ' ';

    if (castling == 0)
    {
		*fen++ = '-';
	}
    else
    {
        if (castling & Seraphina::CastlingType::WKSC)
        {
			*fen++ = Board::isChess960() ? 'A' + ((castling & 1) & 7) : 'K';
		}

        if (castling & Seraphina::CastlingType::WKLC)
        {
            *fen++ = isChess960() ? 'A' + ((castling & (1 << 1)) & 7) : 'Q';
		}

        if (castling & Seraphina::CastlingType::BKSC)
        {
            *fen++ = isChess960() ? 'a' + ((castling & (1 << 2)) & 7) : 'k';
		}

        if (castling & Seraphina::CastlingType::BKLC)
        {
            *fen++ = isChess960() ? 'a' + ((castling & (1 << 3)) & 7) : 'q';
		}
	}

    *fen++ = ' ';

    sprintf(fen, "%s %d %d", getBoardInfo()->ENPASSNT ? SquareCoor[getBoardInfo()->ENPASSNT] : "-", getFifty(), getMoveNum());
}

Seraphina::Square Board::getKingSquare(Seraphina::Color pov) const
{
    return Seraphina::Bitboards::lsb(pieceBB[Seraphina::makepiece(pov, Seraphina::PieceList::KING)]);
}

bool Board::isChecked(Seraphina::Color pov) const
{
    return isAttacked(pov, getKingSquare(pov));
}

bool Board::isAttacked(Seraphina::Color pov, Seraphina::Square sq) const
{
    return Seraphina::Bitboards::getPawnAttacks(pov, sq) != 0
        || Seraphina::Bitboards::getKnightAttacks(sq) != 0
        || Seraphina::Bitboards::getBishopAttacks(sq, occBB[pov]) != 0
        || Seraphina::Bitboards::getRookAttacks(sq, occBB[pov]) != 0
        || Seraphina::Bitboards::getQueenAttacks(sq, occBB[pov]) != 0
        || Seraphina::Bitboards::getKingAttacks(sq) != 0;
}

Bitboard Board::attackersTo(Seraphina::Square sq, Bitboard occupied) const
{
    return (Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::BLACK, sq) & pieceBB[Seraphina::PieceType::WHITE_PAWN])
		| (Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::WHITE, sq) & pieceBB[Seraphina::PieceType::BLACK_PAWN])
		| (Seraphina::Bitboards::getKnightAttacks(sq) & (pieceBB[Seraphina::PieceType::WHITE_KNIGHT] | pieceBB[Seraphina::PieceType::BLACK_KNIGHT]))
		| (Seraphina::Bitboards::getBishopAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_BISHOP] | pieceBB[Seraphina::PieceType::BLACK_BISHOP]))
		| (Seraphina::Bitboards::getRookAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_ROOK] | pieceBB[Seraphina::PieceType::BLACK_ROOK]))
		| (Seraphina::Bitboards::getQueenAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_QUEEN] | pieceBB[Seraphina::PieceType::BLACK_QUEEN]))
		| (Seraphina::Bitboards::getKingAttacks(sq) & (pieceBB[Seraphina::PieceType::WHITE_KING] | pieceBB[Seraphina::PieceType::BLACK_KING]));
}

bool Board::isDraw(MoveList& movelist, int ply)
{
    if (getFifty() > 99 && (!checkers || movelist.getSize()))
    {
        return true;
    }

    return getRepetition() && getRepetition() < ply;
}

bool Board::isRepeated()
{
    int end = std::min(getFifty(), getpliesFromNull());
    int repetitions = getRepetition();

    while (end-- >= 4)
    {
		if (repetitions)
		{
			return true;
		}

        // repetitions = history[history.size() - 2].repetition;
        repetitions = (*(history.rend() + 1)).repetition;
    }

    return false;
}

void Board::printBoard()
{
    static char fen[128];

    std::cout << "+---+---+---+---+---+---+---+---+\n";

    for (int rank = Seraphina::Rank::RANK_1; rank <= Seraphina::Rank::RANK_8; rank++)
    {
        for (int file = Seraphina::File::FILE_A; file <= Seraphina::File::FILE_H; file++)
        {
			const Seraphina::Square sq = Seraphina::makesquare((Seraphina::File)file, (Seraphina::Rank)rank);
			const Seraphina::PieceType pt = getPieceType(sq);

            if (pt == Seraphina::PieceType::NO_PIECETYPE)
            {
				std::cout << "|   ";
			}
            else
            {
				std::cout << "| " << Seraphina::piecetochar(pt) << " ";
			}
		}

        std::cout << "|" << (8 - rank) << "\n";

		std::cout << "+---+---+---+---+---+---+---+---+\n";
	}

    std::cout << "  a   b   c   d   e   f   g   h\n";

    BoardtoFEN(fen);
    std::cout << "\n" << "FEN: " << fen << "\n\n";
}

void Board::ClearBoard()
{
    // Clear the board
    for (int sq = 0; sq < SQ_NUM; sq++)
    {
		board[sq] = Seraphina::PieceType::NO_PIECETYPE;
	}

	// Clear the piece bitboards
    for (int pt = 0; pt < 12; pt++)
    {
		pieceBB[pt] = 0ULL;
	}

	// Clear the occupancy bitboard
    for (int pov = 0; pov < 3; pov++)
    {
        occBB[pov] = 0ULL;
    }

    setPOV(Seraphina::Color::NO_COLOR);
    checkers = 0ULL;
    pinned = 0ULL;
    threatened = 0ULL;

    for (int p = 0; p < 6; p++)
    {
        threatenedBy[p] = 0ULL;
    }

    KingSQ = 0;
    movenum = 0;

    BoardInfo boardinfo;
    history.clear();
    history.shrink_to_fit();
    history.emplace_back(boardinfo);
}