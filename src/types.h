// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#if defined(_MSC_VER) && defined(_WIN64) // MSVC, WIN64
#include <intrin.h>
#endif

// Engine Name
#define Name "Seraphina"

// Engine Official Release Version
#define Version "dev-NNUE-S6"

// Author Name
#define Author "Henry Z"

#define Bold(unix) (unix ? "\033[1m" : "")
#define Blue(unix) (unix ? "\033[34m" : "")
#define Cyan(unix) (unix ? "\033[36m" : "")
#define Vanilla(unix) (unix ? "\033[0m" : "")

// Square Number of the chess board
#define SQ_NUM 64

#define MAX_MOVES 256
#define MAX_PLY 256
#define MAX_THREADS 1024

// Start Position FEN
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

constexpr char PIECETYPE_CHARS[] = "PNBRQKpnbrqk ";
constexpr char PIECE_CHARS[] = "PNBRQK";

//  Unsigned 64 bits
using U64 = uint64_t;

// Divided Definition in order to make codes cleaner
using Bitboard = uint64_t;

// Use Unsigned 32 bits to store move-related information
using Move = uint32_t;

// Use Signed 8 bits to store score information
using Score = int8_t;

namespace Seraphina
{
    // Colors on the board
    enum Color : int
    {
        WHITE, BLACK, NO_COLOR
    };

	// Define the chess board
	enum Square : int
	{
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
        NO_SQ
	};

    // File A ~ H
    enum File : int
    {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NO_FILE
    };

    // Rank 1 ~ 8
    enum Rank : int
    {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NO_RANK
    };

    // Piece List
    enum PieceList : int
    {
        PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE
    };

    // Piece Type with Colors
    enum PieceType : int
    {
        WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
        BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
        NO_PIECETYPE
    };

    // Castling Type
    // We generally use 4 bits to store the castling permission
    //  0    0    0    0
    // WKSC WKLC BKSC BKLC
    enum CastlingType : int
    {
        WKSC = 1, WKLC = 2, BKSC = 4, BKLC = 8,
		ALL_CASTLING = 15
    };

    // Move Types
    enum MoveType : int
    {
		NORMAL, SHORT_CASTLE, LONG_CASTLE, CAPTURE, ENPASSNT, PROMOTION, PROMOTION_CAPTURE
    };

    // Values for Search
    enum Value : int
    {
        VALUE_ZERO = 0,
        VALUE_INFINITE = 32001,
		VALUE_MATE = 32000,
    };

    // Directions are used to generate XRay attacks
    // Sometimes we use file and rank hashs to generate XRay attacks
    enum Direction : int
    {
        NORTH = 8,
        WEST = -1,
        SOUTH = -8,
        EAST = 1,
        NORTH_EAST = 9,
        NORTH_WEST = 7,
        SOUTH_WEST = -9,
        SOUTH_EAST = -7
    };

    constexpr PieceType chartopiece(char f)
    {
        if (f == 'P') return WHITE_PAWN;
        if (f == 'N') return WHITE_KNIGHT;
        if (f == 'B') return WHITE_BISHOP;
        if (f == 'R') return WHITE_ROOK;
        if (f == 'Q') return WHITE_QUEEN;
        if (f == 'K') return WHITE_KING;

        if (f == 'p') return BLACK_PAWN;
        if (f == 'n') return BLACK_KNIGHT;
        if (f == 'b') return BLACK_BISHOP;
        if (f == 'r') return BLACK_ROOK;
        if (f == 'q') return BLACK_QUEEN;
        if (f == 'k') return BLACK_KING;

		return NO_PIECETYPE;
    }

    constexpr char piecetochar(PieceType pt)
    {
        /*
        if (pt == WHITE_PAWN) return 'P';
        if (pt == WHITE_KNIGHT) return 'N';
        if (pt == WHITE_BISHOP) return 'B';
        if (pt == WHITE_ROOK) return 'R';
        if (pt == WHITE_QUEEN) return 'Q';
        if (pt == WHITE_KING) return 'K';

        if (pt == BLACK_PAWN) return 'p';
        if (pt == BLACK_KNIGHT) return 'n';
        if (pt == BLACK_BISHOP) return 'b';
        if (pt == BLACK_ROOK) return 'r';
        if (pt == BLACK_QUEEN) return 'q';
        if (pt == BLACK_KING) return 'k';

        if (pt == NO_PIECETYPE) return ' ';
        */

        return PIECETYPE_CHARS[pt];
	}

    constexpr PieceList chartopieceNOPOV(char f)
    {
        if (f == 'P' || f == 'p') return PAWN;
        if (f == 'N' || f == 'n') return KNIGHT;
        if (f == 'B' || f == 'b') return BISHOP;
        if (f == 'R' || f == 'r') return ROOK;
        if (f == 'Q' || f == 'q') return QUEEN;
        if (f == 'K' || f == 'k') return KING;

		return NO_PIECE;
    }

    constexpr char piecetocharNOPOV(PieceList p)
    {
        /*
        if (p == PAWN) return 'P';
        if (p == KNIGHT) return 'N';
        if (p == BISHOP) return 'B';
        if (p == ROOK) return 'R';
        if (p == QUEEN) return 'Q';
        if (p == KING) return 'K';
        */

        return PIECETYPE_CHARS[p];
    }

    constexpr Square makesquare(File f, Rank r)
	{
        return (Square)(f + r * 8);
    }

    constexpr Square makesquare(int f, int r)
    {
        return (Square)(f + r * 8);
    }

    constexpr File getfile(Square s)
	{
		return (File)(s & 7);
	}

    constexpr File getfile(int s)
    {
        return (File)(s & 7);
    }

    constexpr Rank getrank(Square s)
    {
        return (Rank)(s >> 3);
    }

    constexpr Rank getrank(int s)
    {
        return (Rank)(s >> 3);
    }

    constexpr PieceType makepiece(Color c, PieceList p)
    {
		return (PieceType)(c * 6 + p);
	}

    constexpr PieceType makepiece(int c, PieceList p)
    {
        return (PieceType)(c * 6 + p);
    }

    constexpr PieceType makepiece(Color c, int p)
    {
        return (PieceType)(c * 6 + p);
    }

    constexpr PieceType makepiece(int c, int p)
    {
        return (PieceType)(c * 6 + p);
    }

    constexpr Color getcolor(PieceType pt)
    {
		return (Color)(pt / 6);
	}

    constexpr Color getcolor(int pt)
    {
        return (Color)(pt / 6);
    }

    constexpr PieceList getpiece(PieceType pt)
    {
		return (PieceList)(pt % 6);
	}

    constexpr PieceList getpiece(int pt)
    {
        return (PieceList)(pt % 6);
    }
}