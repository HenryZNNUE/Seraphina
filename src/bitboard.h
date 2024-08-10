#pragma once

// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include <vector>

#include "types.h"
#include "sliders.hpp"

#define FileABB 0x0101010101010101ULL
#define FileBBB 0x0202020202020202ULL
#define FileCBB 0x0404040404040404ULL
#define FileDBB 0x0808080808080808ULL
#define FileEBB 0x1010101010101010ULL
#define FileFBB 0x2020202020202020ULL
#define FileGBB 0x4040404040404040ULL
#define FileHBB 0x8080808080808080ULL

#define Rank1BB 0xFF00000000000000ULL
#define Rank2BB 0x00FF000000000000ULL
#define Rank3BB 0x0000FF0000000000ULL
#define Rank4BB 0x000000FF00000000ULL
#define Rank5BB 0x00000000FF000000ULL
#define Rank6BB 0x0000000000FF0000ULL
#define Rank7BB 0x000000000000FF00ULL
#define Rank8BB 0x00000000000000FFULL

// pre calculated lookup table for pawn attacks
static constexpr Bitboard PAWN_ATTACKS_TABLE[2][64] = {
    // white pawn attacks
    { 0x200, 0x500, 0xa00, 0x1400,
      0x2800, 0x5000, 0xa000, 0x4000,
      0x20000, 0x50000, 0xa0000, 0x140000,
      0x280000, 0x500000, 0xa00000, 0x400000,
      0x2000000, 0x5000000, 0xa000000, 0x14000000,
      0x28000000, 0x50000000, 0xa0000000, 0x40000000,
      0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
      0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
      0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
      0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
      0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
      0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000,
      0x200000000000000, 0x500000000000000, 0xa00000000000000, 0x1400000000000000,
      0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000,
      0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0 },

      // black pawn attacks
      { 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x2, 0x5, 0xa, 0x14,
        0x28, 0x50, 0xa0, 0x40,
        0x200, 0x500, 0xa00, 0x1400,
        0x2800, 0x5000, 0xa000, 0x4000,
        0x20000, 0x50000, 0xa0000, 0x140000,
        0x280000, 0x500000, 0xa00000, 0x400000,
        0x2000000, 0x5000000, 0xa000000, 0x14000000,
        0x28000000, 0x50000000, 0xa0000000, 0x40000000,
        0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
        0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
        0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
        0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
        0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
        0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000
      }
};

static constexpr Bitboard KNIGHT_ATTACKS_TABLE[64] = {
    0x0000000000020400, 0x0000000000050800, 0x00000000000A1100, 0x0000000000142200, 0x0000000000284400,
    0x0000000000508800, 0x0000000000A01000, 0x0000000000402000, 0x0000000002040004, 0x0000000005080008,
    0x000000000A110011, 0x0000000014220022, 0x0000000028440044, 0x0000000050880088, 0x00000000A0100010,
    0x0000000040200020, 0x0000000204000402, 0x0000000508000805, 0x0000000A1100110A, 0x0000001422002214,
    0x0000002844004428, 0x0000005088008850, 0x000000A0100010A0, 0x0000004020002040, 0x0000020400040200,
    0x0000050800080500, 0x00000A1100110A00, 0x0000142200221400, 0x0000284400442800, 0x0000508800885000,
    0x0000A0100010A000, 0x0000402000204000, 0x0002040004020000, 0x0005080008050000, 0x000A1100110A0000,
    0x0014220022140000, 0x0028440044280000, 0x0050880088500000, 0x00A0100010A00000, 0x0040200020400000,
    0x0204000402000000, 0x0508000805000000, 0x0A1100110A000000, 0x1422002214000000, 0x2844004428000000,
    0x5088008850000000, 0xA0100010A0000000, 0x4020002040000000, 0x0400040200000000, 0x0800080500000000,
    0x1100110A00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010A000000000,
    0x2000204000000000, 0x0004020000000000, 0x0008050000000000, 0x00110A0000000000, 0x0022140000000000,
    0x0044280000000000, 0x0088500000000000, 0x0010A00000000000, 0x0020400000000000 };

// pre calculated lookup table for king attacks
static constexpr Bitboard KING_ATTACKS_TABLE[64] = {
    0x0000000000000302, 0x0000000000000705, 0x0000000000000E0A, 0x0000000000001C14, 0x0000000000003828,
    0x0000000000007050, 0x000000000000E0A0, 0x000000000000C040, 0x0000000000030203, 0x0000000000070507,
    0x00000000000E0A0E, 0x00000000001C141C, 0x0000000000382838, 0x0000000000705070, 0x0000000000E0A0E0,
    0x0000000000C040C0, 0x0000000003020300, 0x0000000007050700, 0x000000000E0A0E00, 0x000000001C141C00,
    0x0000000038283800, 0x0000000070507000, 0x00000000E0A0E000, 0x00000000C040C000, 0x0000000302030000,
    0x0000000705070000, 0x0000000E0A0E0000, 0x0000001C141C0000, 0x0000003828380000, 0x0000007050700000,
    0x000000E0A0E00000, 0x000000C040C00000, 0x0000030203000000, 0x0000070507000000, 0x00000E0A0E000000,
    0x00001C141C000000, 0x0000382838000000, 0x0000705070000000, 0x0000E0A0E0000000, 0x0000C040C0000000,
    0x0003020300000000, 0x0007050700000000, 0x000E0A0E00000000, 0x001C141C00000000, 0x0038283800000000,
    0x0070507000000000, 0x00E0A0E000000000, 0x00C040C000000000, 0x0302030000000000, 0x0705070000000000,
    0x0E0A0E0000000000, 0x1C141C0000000000, 0x3828380000000000, 0x7050700000000000, 0xE0A0E00000000000,
    0xC040C00000000000, 0x0203000000000000, 0x0507000000000000, 0x0A0E000000000000, 0x141C000000000000,
    0x2838000000000000, 0x5070000000000000, 0xA0E0000000000000, 0x40C0000000000000 };

class MoveList;

namespace Seraphina
{
    namespace NNUE
    {
        struct alignas(64) Accumulator;
        struct alignas(64) AccumulatorRefreshTable;
    }

	namespace Bitboards
	{

        constexpr Bitboard bit(int sq)
        {
            return (1ULL << sq);
        }

        constexpr Bitboard bit(Square sq)
        {
            return (1ULL << sq);
        }

        // Compiler specific functions, taken from Stockfish https://github.com/official-stockfish/Stockfish
#if defined(__GNUC__) // GCC, Clang, ICC

        inline Square lsb(U64 b)
        {
            if (!b)
                return NO_SQ;
            return Square(__builtin_ctzll(b));
        }

        inline Square msb(U64 b)
        {
            if (!b)
                return NO_SQ;
            return Square(63 ^ __builtin_clzll(b));
        }

#elif defined(_MSC_VER) // MSVC

        inline Square lsb(U64 b)
        {
            unsigned long idx;
            _BitScanForward64(&idx, b);
            return (Square)idx;
        }

        inline Square msb(U64 b)
        {
            unsigned long idx;
            _BitScanReverse64(&idx, b);
            return (Square)idx;
        }

#else

#error "Compiler not supported."

#endif

        inline uint8_t popcount(U64 mask)
        {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)

            return (uint8_t)_mm_popcnt_u64(mask);

#else

            return __builtin_popcountll(mask);

#endif
        }

        inline Square poplsb(U64& mask)
        {
            int8_t s = lsb(mask);
            mask &= mask - 1;
            return Square(s);
        }

        inline void setBit(U64& mask, Square sq)
		{
			mask |= (1ULL << sq);
		}

        inline void popBit(U64& mask, Square sq)
        {
            mask &= ~(1ULL << sq);
        }

        Bitboard getPawnAttacks(Color pov, Square sq);
        Bitboard getKnightAttacks(Square sq);
        Bitboard getBishopAttacks(Square sq, Bitboard occ);
        Bitboard getRookAttacks(Square sq, Bitboard occ);
        Bitboard getQueenAttacks(Square sq, Bitboard occ);
        Bitboard getKingAttacks(Square sq);
        Bitboard getPieceAttacks(Square sq, Bitboard occ, PieceList p);

        void printBB(const Bitboard& bb);

        template <Direction D>
        Bitboard shift(Bitboard bb)
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
	}
}

struct BoardInfo
{
    U64 zobrist = 0ULL;
    U64 pawnZobrist = 0ULL;
    int ENPASSNT = Seraphina::Square::NO_SQ;
    int capture = 0;
    int repetition = 0;
    int fifty = 0;
    int pliesFromNull = 0;
    bool chess960 = false;

    /*
    BoardInfo() : zobrist(0ULL), ENPASSNT(Seraphina::Square::NO_SQ), castling(0),
        repetition(0), fifty(0), move(NULL), movenum(0),
        chess960(false) {}
    */
};

class Board
{
private:
    Bitboard board[SQ_NUM];
    Bitboard pieceBB[Seraphina::PieceType::NO_PIECETYPE];
    Bitboard occBB[Seraphina::Color::NO_COLOR + 1];
	Bitboard SQbtw[SQ_NUM][SQ_NUM];
	int pieceCount[Seraphina::PieceType::NO_PIECETYPE];
	int nonPawns[Seraphina::Color::NO_COLOR];

    Seraphina::Color currentPlayer;
    Bitboard checkers;
    Bitboard pinned;
    Bitboard threatened;
    Bitboard threatenedBy[6];
    int KingSQ;
    int movenum;

    std::vector<BoardInfo> history;

    U64 Zobrist[Seraphina::PieceType::NO_PIECETYPE][SQ_NUM];
    U64 ZobristEP[Seraphina::File::NO_FILE];
    U64 ZobristCastle[16];
    U64 ZobristSide;

    std::array<U64, 8192>  cuckoo;
    std::array<Move, 8192> cuckooMove;

public:
    Seraphina::NNUE::Accumulator* acc;
    Seraphina::NNUE::AccumulatorRefreshTable* accRT;

    int castling = 0;
    int castling_rights[64] = {
         7, 15, 15, 15,  3, 15, 15, 11,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        13, 15, 15, 15, 12, 15, 15, 14
    };

    inline BoardInfo* getBoardInfo()
    {
        return &history.back();
    }

    U64 randU64();
    void initZobrist();
    void generatePawnZobrist();
    void generateZobrist();
    void updateZobrist(const Move& move);

    inline int H1(U64 h)
    {
        return h & 0x1fff;
    }

    inline int H2(U64 h)
    {
        return (h >> 16) & 0x1fff;
    }

    void initCuckoo();

    inline Seraphina::PieceType getBoard(Seraphina::Square sq) const
    {
        return (Seraphina::PieceType)board[sq];
    }

    inline U64 getPieceBB(Seraphina::PieceType pt) const
    {
        return pieceBB[pt];
    }

    /*
    inline void FlipPieceBB(Seraphina::PieceType pt, Seraphina::Square from, Seraphina::Square to)
    {
        pieceBB[pt] ^= from ^ to;
    }
    */

    inline U64 getoccBB(Seraphina::Color pov) const
    {
        return occBB[pov];
    }

    /*
    inline void FlipOccBB(Seraphina::Color pov, Seraphina::Square from, Seraphina::Square to)
    {
        occBB[pov] ^= from ^ to;
    }
    */

	inline U64 getSQbtw(Seraphina::Square sq1, Seraphina::Square sq2) const
	{
		return SQbtw[sq1][sq2];
	}

    void initSQbtw();

    inline Seraphina::Color currPOV() const
    {
        return currentPlayer;
    }

    inline void setPOV(Seraphina::Color pov)
    {
		currentPlayer = pov;
	}

	inline Bitboard getCheckers() const
	{
		return checkers;
	}

    inline std::vector<BoardInfo> getHistory()
    {
        return history;
    }

    inline BoardInfo getPreviousHistory()
    {
        return history[history.size() - 2];
    }

    inline BoardInfo getHistorybyIndex(int index)
    {
        return history[index];
    }

    inline int getHistorySize()
    {
        return history.size();
    }

    inline void setHistory(std::vector<BoardInfo>& hist)
    {
        history = hist;
    }

    inline void setHistorybyIndex(BoardInfo& hist, int index)
    {
        history[index] = hist;
    }

    inline void setHistoryIncremental(BoardInfo& hist)
    {
        history.emplace_back(hist);
    }
    
    inline void undoHistoryIncremental()
	{
		history.pop_back();
	}

    inline U64 getZobrist()
    {
        return getBoardInfo()->zobrist;
    }

    inline int getRepetition()
    {
        return getBoardInfo()->repetition;
    }

    inline void setRepitition(int newRepitition)
    {
        getBoardInfo()->repetition = newRepitition;
    }

    inline void setRepititionIncremental()
    {
        getBoardInfo()->repetition += 1;
    }

    inline int getFifty()
    {
        return getBoardInfo()->fifty >> 1;
    }

    inline void setFifty(int newFifty)
    {
        getBoardInfo()->fifty = newFifty;
    }

    inline void setFiftyIncremental()
    {
        getBoardInfo()->fifty += 1;
    }

    inline void undoFiftyIncremental()
    {
        getBoardInfo()->fifty -= 1;
    }

    inline int getMoveNum()
    {
        return movenum;
    }

    inline void setMoveNum(int newMoveNum)
    {
        movenum = newMoveNum;
    }

    inline void setMoveNumIncremental()
    {
        movenum++;
    }

    inline void undoMoveNumIncremental()
    {
        movenum--;
    }

    inline int getpliesFromNull()
	{
		return getBoardInfo()->pliesFromNull;
	}

    inline void setpliesFromNull(int pn)
    {
        getBoardInfo()->pliesFromNull = pn;
    }

    inline void setpliesFromNullIncremental()
	{
        getBoardInfo()->pliesFromNull += 1;
	}

    inline void undopliesFromNullIncremental()
    {
        getBoardInfo()->pliesFromNull -= 1;
    }

    inline bool isChess960()
    {
        return getBoardInfo()->chess960;
    }

    inline void setChess960(bool chess960)
    {
		getBoardInfo()->chess960 = chess960;
	}

    inline Bitboard getThreatened()
	{
		return threatened;
	}

    inline Bitboard getThreatenedBy(Seraphina::PieceList p)
    {
        return threatenedBy[p];
    }

    inline int getCastlingRights(int castlingtype)
    {
        return (castling & Seraphina::Bitboards::bit(castlingtype));
    }

    Seraphina::PieceType getPieceType(Seraphina::Square sq) const;
    void setPiece(Seraphina::Square sq, Seraphina::PieceType pt);
    void removePiece(Seraphina::Square sq);
    void replacePiece(Seraphina::Square sq, Seraphina::PieceType pt);
    void setCPK();
    void setThreats();

    int getPieceValue(Seraphina::PieceType pt) const;
    int countPiece(Seraphina::PieceType pt);
	int countPieceValues(Seraphina::Color pov);

    void parseFEN(char* fen);
    void BoardtoFEN(char* fen);

    Seraphina::Square getKingSquare(Seraphina::Color pov) const;

    bool isChecked(Seraphina::Color pov) const;

    bool isAttacked(Seraphina::Color pov, Seraphina::Square sq) const;
    Bitboard attackersTo(Seraphina::Square sq, Bitboard occupied) const;

    bool isDraw(MoveList& movelist, int ply);
    bool isRepeated();

    void printBoard();

    void ClearBoard();
};