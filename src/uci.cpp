// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "uci.h"
#include "types.h"
#include "bitboard.h"

#include <cmath>

// Return a = p_a(material) and b = p_b(material), see github.com/official-stockfish/WDL_model
constexpr double as[] = { -41.25712052, 121.47473115, -124.46958843, 411.84490997 };
constexpr double bs[] = { 84.92998051, -143.66658718, 80.09988253, 49.80869370 };

namespace Seraphina
{
	static void UCI_Intro()
	{
		std::cout << "id name " << NAME << VERSION << "\n";
		std::cout << "id author " << Author << "\n";
		std::cout << "uciok\n";
	}

	struct WDLParams
	{
		double a, b;
	};

	WDLParams WDL(Board& board)
	{
		int material = board.countPiece(Seraphina::PieceType::WHITE_PAWN) + board.countPiece(Seraphina::PieceType::BLACK_PAWN)
			+ 3 * (board.countPiece(Seraphina::PieceType::WHITE_KNIGHT) + board.countPiece(Seraphina::PieceType::BLACK_KNIGHT))
			+ 3 * (board.countPiece(Seraphina::PieceType::WHITE_BISHOP) + board.countPiece(Seraphina::PieceType::BLACK_BISHOP))
			+ 5 * (board.countPiece(Seraphina::PieceType::WHITE_ROOK) + board.countPiece(Seraphina::PieceType::BLACK_ROOK))
			+ 9 * (board.countPiece(Seraphina::PieceType::WHITE_QUEEN) + board.countPiece(Seraphina::PieceType::BLACK_QUEEN));

		double m = std::clamp(material, 17, 78) / 58.0;

		double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
		double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

		return {a, b};
	}

	int getWDL(Board& board, Score score)
	{
		auto [a, b] = WDL(board);

		return int(0.5 + 1000 / (1 + std::exp((a - double(score)) / b)));
	}
}