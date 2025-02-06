// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include "types.h"
#include <vector>

/*
* A Detailed decription of Seraphina's Move storage philosophy
* Modified from Koivisto by Luecx
* 
* 00000000 0000 0000 0000 000000 000000
* |        |    |    |    |      |      Square From
* |        |    |    |    |      +------
* |        |    |    |    |             Square To
* |        |    |    |    +-------------
* |        |    |    |                  Moving Piece Type
* |        |    |    +------------------
* |        |    |                       Move Type
* |        |    +-----------------------
* |        |                            Captured / Promoted Piece
* |        +----------------------------
* |                                     Score
* +-------------------------------------
*/

class Board;

namespace Seraphina
{
	// Thanks to Luecx's Koivisto codes,
	// which let me learn the necessity of "inline"
	// that can help avoid the frequent stack operation
	// Also, use const reference here which can avoid copying, improve performance
	// Here are "get" functions
	inline Square getFrom(const Move& move)
	{
		// Explaination: squareFrom is located at 0 ~ 6 bits in a move
		// therefore there is no need to right shift the move in order to align the required part
		// (1ULL << 6) could be: 1111 ... 1111 1100 0000
		// after - 1: 1111 ... 1111 1011 1111
		// as move undergos bitwise and, it will be like this if From Square is E1: ... 0100
		// as (Square) type is int, therefore, the result will be 4
		// Cool! Isn't it?
		// static_cast<type>(data); is used to convert data type
		return (Square)(move & ((1ULL << 6) - 1));
	}

	inline Square getTo(const Move& move)
	{
		// Explaination: squareFrom is located at 6 ~ 12 bits in a move
		// it's necessary to right shift the move by 6 in order to align the required part
		// like this:     1111      ->     1111
		//            (6 ~ 12 bits)    (0 ~ 6 bits)
		// (1ULL << 6) could be: 1111 ... 1111 1100 0000
		// after - 1: 1111 ... 1111 1011 1111
		// as move undergos bitwise and, it will be like this if To Square is E1: ... 0100
		// as (Square) type is int, therefore, the result will be 4
		// Cool! Isn't it?
		// static_cast<type>(data); is used to convert data type
		return (Square)((move >> 6) & ((1ULL << 6) - 1));
	}

	inline int getFromTo(const Move& move)
	{
		return (int)(move & ((1ULL << 12) - 1));
	}

	inline PieceType getPieceType(const Move& move)
	{
		return (PieceType)((move >> 12) & ((1ULL << 4) - 1));
	}

	inline MoveType getMoveType(const Move& move)
	{
		return (MoveType)((move >> 16) & ((1ULL << 4) - 1));
	}

	inline PieceType getCaptured(const Move& move)
	{
		return (PieceType)((move >> 20) & ((1ULL << 4) - 1));
	}

	inline PieceType getCapPromo(const Move& move)
	{
		return (PieceType)((move >> 20) & ((1ULL << 4) - 1));
	}

	inline Score getScore(const Move& move)
	{
		return (Score)((move >> 24) & ((1ULL << 8) - 1));
	}

	// Here are "set" functions
	inline void setFrom(Move& move, Square from)
	{
		move |= from;
	}

	inline void setFrom(Move& move, int from)
	{
		move |= from;
	}

	inline void setTo(Move& move, Square to)
	{
		move |= (to << 6);
	}

	inline void setTo(Move& move, int to)
	{
		move |= (to << 6);
	}

	inline void setPieceType(Move& move, PieceType pt)
	{
		move |= (pt << 12);
	}

	inline void setPieceType(Move& move, int pt)
	{
		move |= (pt << 12);
	}

	inline void setMoveType(Move& move, MoveType mt)
	{
		move |= (mt << 16);
	}

	inline void setMoveType(Move& move, int mt)
	{
		move |= (mt << 16);
	}

	inline void setCapPromo(Move& move, PieceType cpt)
	{
		move |= (cpt << 20);
	}

	inline void setCapPromo(Move& move, int cpt)
	{
		move |= (cpt << 20);
	}

	inline void setScore(Move& move, Score score)
	{
		move |= (score << 24);
	}

	// There are 2 types of set move, this one refers to non-capture moves
	inline Move setMove(const Square& from, const Square& to, const PieceType& pt, const MoveType& mt)
	{
		Move move{ 0 }; // Initialize move's block of memory

		// use "set" functions defined in movegen.h to fill the move-related information
		setFrom(move, from);
		setTo(move, to);
		setPieceType(move, pt);
		setMoveType(move, mt);
		// setScore(move, score);

		return move;
	}

	// There are 2 types of set move, this one refers to non-capture moves
	inline Move setMove(const int& from, const int& to, const int& pt, const MoveType& mt)
	{
		Move move{ 0 }; // Initialize move's block of memory

		// use "set" functions defined in movegen.h to fill the move-related information
		setFrom(move, from);
		setTo(move, to);
		setPieceType(move, pt);
		setMoveType(move, mt);
		// setScore(move, score);

		return move;
	}

	// There are 2 types of set move, this one refers to capture / promotion moves
	inline Move setMove(const Square& from, const Square& to, const PieceType& pt, const MoveType& mt, const PieceType& cappromo)
	{
		Move move{ 0 }; // Initialize move's block of memory

		// use "set" functions defined in movegen.h to fill the move-related information
		setFrom(move, from);
		setTo(move, to);
		setPieceType(move, pt);
		setMoveType(move, mt);
		setCapPromo(move, cappromo);
		// setScore(move, score);

		return move;
	}

	// There are 2 types of set move, this one refers to capture / promotion moves
	inline Move setMove(const int& from, const int& to, const int& pt, const MoveType& mt, const int& cappromo)
	{
		Move move{ 0 }; // Initialize move's block of memory

		// use "set" functions defined in movegen.h to fill the move-related information
		setFrom(move, from);
		setTo(move, to);
		setPieceType(move, pt);
		setMoveType(move, mt);
		setCapPromo(move, cappromo);
		// setScore(move, score);

		return move;
	}

	void makemove(Board& board, const Move& move, bool update = true);
	void undomove(Board& board, const Move& move);

	std::string movetoString(Board& board, const Move& move);

	bool isPseudoLegal(Board& board, const Move& move);
}

class MoveList
{
private:
	std::vector<Move> moves;

public:
	inline std::vector<Move> getMoves() const
	{
		return moves;
	}

	inline Move getMove(int num) const
	{
		return moves[num];
	}

	inline int getSize() const
	{
		return moves.size();
	}

	bool contains(const Move& move) const
	{
		return std::find(moves.begin(), moves.end(), move) != moves.end();
	}

	void generatePawn(Board& board, Seraphina::MoveType mt, Bitboard& target, Bitboard& checkers);
	void generateKing(Board& board, Seraphina::MoveType mt, Bitboard& target, Bitboard& checkers);
	template <Seraphina::PieceList p>
	void generatebyPieceType(Board& board, Seraphina::MoveType mt, Bitboard& target);
	template<Seraphina::MoveType mt>
	void generatePseudoLegal(Board& board);

	void generateQuiet(Board& board);
	void generateNoisy(Board& board);

	void generateAll(Board& board);

	MoveList()
	{
		moves.reserve(MAX_MOVES);
	}
};