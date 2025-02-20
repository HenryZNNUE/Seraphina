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

constexpr int PieceValue[13] = { 208, 781, 825, 1276, 2538, 32001, 208, 781, 825, 1276, 2538, 32001, 0 };

namespace Seraphina
{
	namespace Bitboards
	{
        Bitboard getPawnAttacks(Color pov, Square sq)
        {
            return PAWN_ATTACKS_TABLE[pov][sq];
        }

        Bitboard getPawnAttacks(int pov, int sq)
        {
            return PAWN_ATTACKS_TABLE[pov][sq];
        }

        Bitboard getKnightAttacks(Square sq)
        {
            return KNIGHT_ATTACKS_TABLE[sq];
        }

        Bitboard getKnightAttacks(int sq)
        {
            return KNIGHT_ATTACKS_TABLE[sq];
        }

        Bitboard getBishopAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::BishopAttacks(sq, occ);
        }

        Bitboard getBishopAttacks(int sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::BishopAttacks(sq, occ);
        }

        Bitboard getRookAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::RookAttacks(sq, occ);
        }

        Bitboard getRookAttacks(int sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::RookAttacks(sq, occ);
        }

        Bitboard getQueenAttacks(Square sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::QueenAttacks(sq, occ);
        }

        Bitboard getQueenAttacks(int sq, Bitboard occ)
        {
            return Chess_Lookup::Fancy::QueenAttacks(sq, occ);
        }

        Bitboard getKingAttacks(Square sq)
        {
            return KING_ATTACKS_TABLE[sq];
        }

        Bitboard getKingAttacks(int sq)
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
                    break;
            }

            return 0;
        }

        Bitboard getPieceAttacks(int sq, Bitboard occ, int p)
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
                break;
            }

            return 0;
        }

        Bitboard shift(Direction D, Bitboard bb)
        {
            return D == Direction::NORTH ? bb << 8
                : D == Direction::SOUTH ? bb >> 8
                : D == Direction::NORTH + Direction::NORTH ? bb << 16
                : D == Direction::SOUTH + Direction::SOUTH ? bb >> 16
                : D == Direction::EAST ? (bb & ~FileHBB) << 1
                : D == Direction::WEST ? (bb & ~FileABB) >> 1
                : D == Direction::NORTH_EAST ? (bb & ~FileHBB) << 9
                : D == Direction::NORTH_WEST ? (bb & ~FileABB) << 7
                : D == Direction::SOUTH_EAST ? (bb & ~FileHBB) >> 7
                : D == Direction::SOUTH_WEST ? (bb & ~FileABB) >> 9
                : 0;
        }

        std::string squaretostr(Square s)
        {
            std::string str = "";

            str += (char)('a' + get_file(s));
            str += (char)('1' + get_rank(s));

            return str;
        }

        std::string squaretostr(int s)
        {
            std::string str = "";

            str += (char)('a' + get_file(s));
            str += (char)('1' + get_rank(s));

            return str;
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
    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; ++pt)
    {
        for (int sq = Seraphina::Square::SQ_A1; sq < Seraphina::Square::NO_SQ; ++sq)
        {
			Zobrist[pt][sq] = randU64();
		}
    }

    for (int file = Seraphina::File::FILE_A; file < Seraphina::File::NO_FILE; ++file)
    {
		ZobristEP[file] = randU64();
    }

    for (int i = 0; i < 16; ++i)
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

    if (pov == Seraphina::Color::BLACK)
    {
        hash ^= ZobristSide;
    }

    getBoardInfo()->pawnZobrist = hash;
}

// Generate Zobrist Hashing
void Board::generateZobrist()
{
	U64 hash = 0ULL;

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; ++pt)
    {
        Bitboard pieces = pieceBB[pt];

        while (pieces)
        {
            hash ^= Zobrist[pt][Seraphina::Bitboards::poplsb(pieces)];
        }
	}

    if (getBoardInfo()->ENPASSNT != Seraphina::Square::NO_SQ)
    {
		hash ^= ZobristEP[getBoardInfo()->ENPASSNT];
	}

	hash ^= ZobristCastle[castling];

    if (pov == Seraphina::Color::BLACK)
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

    if (Seraphina::get_piece(pt) == Seraphina::PieceList::PAWN)
    {
        if (mt == Seraphina::MoveType::CAPTURE || mt == Seraphina::MoveType::PROMOTION_CAPTURE)
        {
            getBoardInfo()->pawnZobrist ^= Zobrist[getBoardInfo()->capture][to];
        }

        else if (mt == Seraphina::MoveType::ENPASSNT)
        {
            int ENPASSNT = to - (pov == Seraphina::Color::WHITE
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

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt <= Seraphina::PieceType::BLACK_KING; ++pt)
	{
        Seraphina::PieceList p = Seraphina::get_piece(pt);

		for (int s1 = Seraphina::Square::SQ_A1; s1 <= Seraphina::Square::SQ_H8; ++s1)
		{
            for (int s2 = s1 + 1; s2 <= Seraphina::Square::SQ_H8; ++s2)
            {
                if ((p != Seraphina::PieceList::PAWN)
                    && (Seraphina::Bitboards::getPieceAttacks(s1, 0, p)
                        & Seraphina::Bitboards::bit(s2)))
                {
                    Move move = Seraphina::setMove(s1, s2, pt, Seraphina::MoveType::NORMAL);

                    U64 key = Zobrist[pt][s1] ^ Zobrist[pt][s2] ^ ZobristSide;
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

                    ++count;
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

    for (int s1 = 0; s1 < 64; ++s1)
    {
        for (int s2 = s1 + 1; s2 < 64; ++s2)
        {
            if (Seraphina::get_rank(s1) == Seraphina::get_rank(s2))
            {
                i = s1 + Seraphina::Direction::EAST;

                while (i < s2)
                {
                    SQbtw[s1][s2] |= Seraphina::Bitboards::bit(i);
                    i += Seraphina::Direction::EAST;
                }
            }
            else if (Seraphina::get_file(s1) == Seraphina::get_file(s2))
            {
                i = s1 + Seraphina::Direction::NORTH;

                while (i < s2)
                {
                    SQbtw[s1][s2] |= Seraphina::Bitboards::bit(i);
                    i += Seraphina::Direction::NORTH;
                }
            }
            else if ((s2 - s1) % Seraphina::Direction::NORTH_EAST == 0
                && (Seraphina::get_file(s1) < Seraphina::get_file(s2)))
            {
                i = s1 + Seraphina::Direction::NORTH_EAST;

                while (i < s2)
                {
                    SQbtw[s1][s2] |= Seraphina::Bitboards::bit(i);
                    i += Seraphina::Direction::NORTH_EAST;
                }
            }
            else if ((s2 - s1) % Seraphina::Direction::NORTH_WEST == 0
                && (Seraphina::get_file(s1) > Seraphina::get_file(s2)))
            {
                i = s1 + Seraphina::Direction::NORTH_WEST;

                while (i < s2)
                {
                    SQbtw[s1][s2] |= Seraphina::Bitboards::bit(i);
                    i += Seraphina::Direction::NORTH_WEST;
                }
            }
        }
    }

    for (int s1 = 0; s1 < 64; ++s1)
    {
        for (int s2 = 0; s2 < s1; ++s2)
        {
            SQbtw[s1][s2] = SQbtw[s2][s1];
        }
    }
}

/*
Seraphina::Direction Board::where_to_push()
{
    return pov == Seraphina::Color::WHITE
        ? Seraphina::Direction::NORTH
        : Seraphina::Direction::SOUTH;
}
*/

int Board::where_to_push()
{
    return (Seraphina::Direction::NORTH
        - ((Seraphina::Direction::NORTH * pov) << 1));
}

Seraphina::PieceType Board::getPieceType(Seraphina::Square sq) const
{
	return static_cast<Seraphina::PieceType>(board[sq]);
}

Seraphina::PieceType Board::getPieceType(int sq) const
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

void Board::setPiece(int sq, int pt)
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

void Board::removePiece(int sq)
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

void Board::replacePiece(int sq, int pt)
{
    removePiece(sq);
    setPiece(sq, pt);
}

// Set checkers & blockers & pinners
void Board::setCBP()
{
	KingSQ = getKingSquare(pov);
	Bitboard pieces = occBB[Seraphina::Color::NO_COLOR];
    Bitboard rq = pieceBB[Seraphina::PieceType::WHITE_ROOK] | pieceBB[Seraphina::PieceType::BLACK_ROOK]
        | pieceBB[Seraphina::PieceType::WHITE_QUEEN] | pieceBB[Seraphina::PieceType::BLACK_QUEEN];

    checkers = attackersTo(KingSQ, occBB[Seraphina::Color::NO_COLOR]) & occBB[~pov];

	blockers[pov] = 0ULL;
	pinners[~pov] = 0ULL;

    Bitboard sliders = (Seraphina::Bitboards::getQueenAttacks(KingSQ, pieces) & rq)
        & occBB[~pov];

	Bitboard occ = occBB[Seraphina::Color::NO_COLOR] ^ sliders;

    while (sliders)
    {
		Seraphina::Square sq = Seraphina::Bitboards::poplsb(sliders);
		Bitboard b = SQbtw[KingSQ][sq] & occ;

        if (b && !Seraphina::Bitboards::more_than_one(b))
        {
			blockers[pov] |= b;

            if (b & occBB[pov])
            {
				pinners[~pov] |= sq;
            }
        }
    }
}

void Board::setThreats()
{
    int other = pov ^ 1;

    Bitboard occ  = occBB[Seraphina::Color::NO_COLOR]
        ^ pieceBB[Seraphina::make_piece(pov, Seraphina::PieceList::KING)];

    Bitboard pawnAttacks = pov ?
        Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_WEST>(pieceBB[Seraphina::PieceType::BLACK_PAWN])
        | Seraphina::Bitboards::shift<Seraphina::Direction::SOUTH_EAST>(pieceBB[Seraphina::PieceType::BLACK_PAWN])
        : Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_WEST>(pieceBB[Seraphina::PieceType::WHITE_PAWN])
        | Seraphina::Bitboards::shift<Seraphina::Direction::NORTH_EAST>(pieceBB[Seraphina::PieceType::WHITE_PAWN]);

    threatenedBy[Seraphina::PieceList::PAWN] = pawnAttacks;
    threatened = threatenedBy[Seraphina::PieceList::PAWN];

    for (int p = Seraphina::PieceList::PAWN; p <= Seraphina::PieceList::QUEEN; ++p)
    {
        Bitboard pieces = pieceBB[Seraphina::make_piece(pov, p)];

        while (pieces)
        {
            const Seraphina::Square sq = Seraphina::Bitboards::poplsb(pieces);

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
        Seraphina::Bitboards::lsb(pieceBB[Seraphina::make_piece(other, Seraphina::PieceList::KING)]));
    threatened |= threatenedBy[Seraphina::PieceList::KING];
}

int Board::getPieceCount(Seraphina::PieceType pt) const
{
	return pieceCount[pt];
}

int Board::getPieceValues(Seraphina::Color c) const
{
    return nonPawns[c];
}

void Board::countPiece()
{
    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt <= Seraphina::PieceType::BLACK_KING; ++pt)
    {
        for (int sq = 0; sq < SQ_NUM; ++sq)
        {
            if (board[sq] != Seraphina::PieceType::NO_PIECETYPE
                && pieceCount[pt] == board[sq])
            {
                ++pieceCount[pt];
            }
        }
    }
}

void Board::countPieceValues()
{
    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt <= Seraphina::PieceType::BLACK_KING; ++pt)
    {
        if (pt != Seraphina::make_piece(pov, Seraphina::PieceList::PAWN)
            && pt != Seraphina::make_piece(pov, Seraphina::PieceList::KING))
        {
            if (pieceCount[pt] != 0)
            {
                nonPawns[pov] += PieceValue[pt] * pieceCount[pt];
            }
        }
    }
}

void Board::addCount(Seraphina::PieceType pt)
{
	++pieceCount[pt];
}

void Board::addCount(int pt)
{
	++pieceCount[pt];
}

void Board::removeCount(Seraphina::PieceType pt)
{
	--pieceCount[pt];
}

void Board::removeCount(int pt)
{
	--pieceCount[pt];
}

void Board::addCountValues(Seraphina::PieceType pt)
{
	nonPawns[Seraphina::get_color(pt)] += PieceValue[pt];
}

void Board::addCountValues(int pt)
{
    nonPawns[Seraphina::get_color(pt)] += PieceValue[pt];
}

void Board::removeCountValues(Seraphina::PieceType pt)
{
    nonPawns[Seraphina::get_color(pt)] -= PieceValue[pt];
}

void Board::removeCountValues(int pt)
{
    nonPawns[Seraphina::get_color(pt)] -= PieceValue[pt];
}

void Board::addCountValues(Seraphina::PieceType pt, Seraphina::Color c)
{
	nonPawns[c] += PieceValue[pt];
}

void Board::removeCountValues(Seraphina::PieceType pt, Seraphina::Color c)
{
	nonPawns[c] -= PieceValue[pt];
}

void Board::parseFEN(char* fen)
{
    ClearBoard();
	int sq = Seraphina::Square::SQ_A8;
    unsigned char token, col, row;
    bool ep = false;
    std::istringstream ss(fen);
    ss >> std::noskipws;
    
	while ((ss >> token) && !isspace(token))
	{
		if (token == '/')
		{
			sq -= 16;
		}
		else if (isdigit(token))
		{
			sq += token - '0';
		}
		else
		{
			setPiece(sq, Seraphina::char_to_piece(token));
			++sq;
		}
	}

    ss >> token;
    token == 'w' ? pov = Seraphina::Color::WHITE : pov = Seraphina::Color::BLACK;

    ss >> token;

    while ((ss >> token) && !isspace(token))
    {

        if (token == 'K')
        {
            castling |= Seraphina::CastlingType::WKSC;
			castlingRookSquare[0] = Seraphina::Bitboards::msb(pieceBB[Seraphina::make_piece(Seraphina::Color::WHITE, Seraphina::PieceList::ROOK)] & Rank1BB);
        }

        else if (token == 'Q')
        {
            castling |= Seraphina::CastlingType::WKLC;
            castlingRookSquare[1] = Seraphina::Bitboards::lsb(pieceBB[Seraphina::make_piece(Seraphina::Color::WHITE, Seraphina::PieceList::ROOK)] & Rank1BB);
        }

        else if (token >= 'A' && token <= 'H')
        {
			castling |= ((token - 'A') > Seraphina::get_file(getKingSquare(Seraphina::Color::WHITE))) ? Seraphina::CastlingType::WKSC : Seraphina::CastlingType::WKLC;
			castlingRookSquare[(token - 'A') > Seraphina::get_file(getKingSquare(Seraphina::Color::WHITE)) ? 0 : 1]
                = Seraphina::make_square((token - 'A'), (Seraphina::Rank::RANK_1 ^ Seraphina::Color::WHITE * 7));
        }

		else if (token == 'k')
		{
			castling |= Seraphina::CastlingType::BKSC;
			castlingRookSquare[2] = Seraphina::Bitboards::msb(pieceBB[Seraphina::make_piece(Seraphina::Color::BLACK, Seraphina::PieceList::ROOK)] & Rank8BB);
		}

		else if (token == 'q')
		{
			castling |= Seraphina::CastlingType::BKLC;
			castlingRookSquare[3] = Seraphina::Bitboards::lsb(pieceBB[Seraphina::make_piece(Seraphina::Color::BLACK, Seraphina::PieceList::ROOK)] & Rank8BB);
		}

		else if (token >= 'a' && token <= 'h')
		{
			castling |= ((token - 'a') > Seraphina::get_file(getKingSquare(Seraphina::Color::BLACK))) ? Seraphina::CastlingType::BKSC : Seraphina::CastlingType::BKLC;
			castlingRookSquare[(token - 'a') > Seraphina::get_file(getKingSquare(Seraphina::Color::BLACK)) ? 2 : 3]
				= Seraphina::make_square((token - 'a'), (Seraphina::Rank::RANK_1 ^ Seraphina::Color::BLACK * 7));
		}
	}

    for (int pt = Seraphina::PieceType::WHITE_PAWN; pt < Seraphina::PieceType::NO_PIECETYPE; pt++)
    {
        occBB[pt & 1] |= pieceBB[pt];
    }

    occBB[Seraphina::Color::NO_COLOR] = occBB[Seraphina::Color::WHITE] | occBB[Seraphina::Color::BLACK];

    if (((ss >> col) && (col >= 'a' && col <= 'h'))
        && ((ss >> row) && (row == (pov == Seraphina::Color::WHITE ? '6' : '3'))))
    {
        int xpov = ~pov;
		int push = where_to_push();
		int xpush = -push;
        getBoardInfo()->ENPASSNT = Seraphina::make_square((col - 'a'), (row - '1'));

		ep = (Seraphina::Bitboards::getPawnAttacks(xpov, Seraphina::make_square((col - 'a'), (row - '1')))
            & pieceBB[Seraphina::make_piece(pov, Seraphina::PieceList::PAWN)])
            && (pieceBB[Seraphina::make_piece(xpov, Seraphina::PieceList::PAWN)]
                & (getBoardInfo()->ENPASSNT + xpush))
            && !(occBB[Seraphina::Color::NO_COLOR] & (getBoardInfo()->ENPASSNT
                | (getBoardInfo()->ENPASSNT + push)));
    }

    if (!ep)
    {
		getBoardInfo()->ENPASSNT = Seraphina::Square::NO_SQ;
    }

    ss >> std::skipws >> getBoardInfo()->fifty >> movenum;

    setCBP();
    setThreats();
    generatePawnZobrist();
	generateZobrist();
}

void Board::BoardtoFEN(char* fen)
{
	int count = 0;
    std::stringstream ss;

    for (int r = Seraphina::Rank::RANK_8; r >= Seraphina::Rank::RANK_1; --r)
    {
        for (int f = Seraphina::File::FILE_A; f <= Seraphina::File::FILE_H; ++f)
        {
            Seraphina::Square sq = Seraphina::make_square(f, r);
            Seraphina::PieceType pt = getPieceType(sq);

            if (pt == Seraphina::PieceType::NO_PIECETYPE)
            {
                ++count;
            }
            else
            {
                if (count)
                {
                    ss << count;
                    count = 0;
                }

                ss << Seraphina::piece_to_char(pt);
            }
        }

        if (count)
        {
            ss << count;
            count = 0;
        }

        if (r > Seraphina::Rank::RANK_1)
        {
            ss << "/";
        }

	}

	ss << " " << (pov == Seraphina::Color::WHITE ? "w" : "b") << " ";

	if (castling & Seraphina::CastlingType::WKSC)
	{
        ss << (getBoardInfo()->chess960
            ? char('A' + Seraphina::get_file(castlingRookSquare[0]))
            : 'K');
	}

	if (castling & Seraphina::CastlingType::WKLC)
	{
        ss << (getBoardInfo()->chess960
            ? char('A' + Seraphina::get_file(castlingRookSquare[1]))
            : 'Q');
	}

	if (castling & Seraphina::CastlingType::BKSC)
	{
        ss << (getBoardInfo()->chess960
            ? char('A' + Seraphina::get_file(castlingRookSquare[2]))
            : 'k');
	}

	if (castling & Seraphina::CastlingType::BKLC)
	{
        ss << (getBoardInfo()->chess960
            ? char('A' + Seraphina::get_file(castlingRookSquare[3]))
            : 'q');
	}

	if (!(castling & (Seraphina::CastlingType::WKSC | Seraphina::CastlingType::WKLC | Seraphina::CastlingType::BKSC | Seraphina::CastlingType::BKLC)))
	{
		ss << "-";
	}

	ss << " ";

	if (getBoardInfo()->ENPASSNT != Seraphina::Square::NO_SQ)
	{
		ss << Seraphina::Bitboards::squaretostr(getBoardInfo()->ENPASSNT);
	}
	else
	{
		ss << "-";
	}

	ss << " " << getFifty() << " " << getMoveNum();

	strcpy(fen, ss.str().c_str());
}

Seraphina::Square Board::getKingSquare(Seraphina::Color c) const
{
    return Seraphina::Bitboards::lsb(pieceBB[Seraphina::make_piece(c, Seraphina::PieceList::KING)]);
}

Seraphina::Square Board::getKingSquare(int c) const
{
    return Seraphina::Bitboards::lsb(pieceBB[Seraphina::make_piece(c, Seraphina::PieceList::KING)]);
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

Bitboard Board::attackersTo(int sq, Bitboard occupied) const
{
    return (Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::BLACK, sq) & pieceBB[Seraphina::PieceType::WHITE_PAWN])
        | (Seraphina::Bitboards::getPawnAttacks(Seraphina::Color::WHITE, sq) & pieceBB[Seraphina::PieceType::BLACK_PAWN])
        | (Seraphina::Bitboards::getKnightAttacks(sq) & (pieceBB[Seraphina::PieceType::WHITE_KNIGHT] | pieceBB[Seraphina::PieceType::BLACK_KNIGHT]))
        | (Seraphina::Bitboards::getBishopAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_BISHOP] | pieceBB[Seraphina::PieceType::BLACK_BISHOP]))
        | (Seraphina::Bitboards::getRookAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_ROOK] | pieceBB[Seraphina::PieceType::BLACK_ROOK]))
        | (Seraphina::Bitboards::getQueenAttacks(sq, occupied) & (pieceBB[Seraphina::PieceType::WHITE_QUEEN] | pieceBB[Seraphina::PieceType::BLACK_QUEEN]))
        | (Seraphina::Bitboards::getKingAttacks(sq) & (pieceBB[Seraphina::PieceType::WHITE_KING] | pieceBB[Seraphina::PieceType::BLACK_KING]));
}

bool Board::isAttacked(Seraphina::Square sq, Bitboard occupied, Seraphina::Color c) const
{
    for (int p = Seraphina::PieceList::PAWN; p < Seraphina::PieceList::KING; ++p)
    {
        if (Seraphina::Bitboards::getPieceAttacks(sq, occupied, p)
            & pieceBB[Seraphina::make_piece(c, p)])
        {
            return true;
        }
    }

    return false;
}

bool Board::isAttacked(int sq, Bitboard occupied, int c) const
{
    for (int p = Seraphina::PieceList::PAWN; p < Seraphina::PieceList::KING; ++p)
    {
        if (Seraphina::Bitboards::getPieceAttacks(sq, occupied, p)
            & pieceBB[Seraphina::make_piece(c, p)])
        {
            return true;
        }
    }

    return false;
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

    for (int rank = Seraphina::Rank::RANK_8; rank >= Seraphina::Rank::RANK_1; --rank)
    {
        for (int file = Seraphina::File::FILE_A; file <= Seraphina::File::FILE_H; ++file)
        {
			const Seraphina::Square sq = Seraphina::make_square(file, rank);
			const Seraphina::PieceType pt = getPieceType(sq);

            if (pt == Seraphina::PieceType::NO_PIECETYPE)
            {
				std::cout << "|   ";
			}
            else
            {
				std::cout << "| " << Seraphina::piece_to_char(pt) << " ";
			}
		}

        std::cout << "|" << (rank + 1) << "\n";

		std::cout << "+---+---+---+---+---+---+---+---+\n";
	}

    std::cout << "  a   b   c   d   e   f   g   h\n";

    BoardtoFEN(fen);
    std::cout << "\n" << "FEN: " << fen << "\n\n";
}

void Board::ClearBoard()
{
    // Clear the board
    for (int sq = 0; sq < SQ_NUM; ++sq)
    {
		board[sq] = Seraphina::PieceType::NO_PIECETYPE;
	}

	// Clear the piece bitboards
    for (int pt = 0; pt < 12; ++pt)
    {
		pieceBB[pt] = 0ULL;
	}

	// Clear the occupancy bitboard
    for (int c = 0; c < 3; ++c)
    {
        occBB[c] = 0ULL;
    }

	nonPawns[0] = 0;
	nonPawns[1] = 0;

    pov = Seraphina::Color::NO_COLOR;

    checkers = 0ULL;

    blockers[0] = 0ULL;
	blockers[1] = 0ULL;

    pinners[0] = 0ULL;
	pinners[1] = 0ULL;

    threatened = 0ULL;

    for (int p = 0; p < 6; ++p)
    {
        pieceCount[p] = 0;
        threatenedBy[p] = 0ULL;
    }

    castling = 0;

    for (int i = 0; i < 4; ++i)
    {
		castlingRookSquare[i] = 0;
    }

    KingSQ = 0;
    movenum = 0;

    BoardInfo boardinfo;
    history.clear();
    history.emplace_back(boardinfo);
}