// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#ifdef _WIN64 // Microsoft Windows Only
#include <Windows.h> // SetConsoleTextAttribute();
#endif // Microsoft Windows & Linux

#include <stdio.h>

#include "uci.h"
#include "bitboard.h"
#include "evaluate.h"
#include "movegen.h"
#include "nnue.h"

namespace Seraphina
{
    // Fancy Terminal
    void UI()
    {
#ifdef _WIN64 // Compatible with Microsoft Windows
        system("title Seraphina");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE);
        printf("=======      =======\n");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("\\\\     ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
        printf("\\\\  //");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("     //\n");
        printf(" \\\\     ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
        printf("\\\\//");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("     //\n");
        printf("  \\\\    ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
        printf("//\\\\");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("    //     Seraphina\n");
        printf("   \\\\  ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
        printf("//  \\\\");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("  //\n");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE);
        printf("     ==      ==\n\n");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN);
#else // Compatible with Linux
        printf("\033[0m\033[1;34m%s\033[0m", "=======      =======\n");
        printf("\033[0m\033[1;36m%s\033[0m", "\\\\     ");
        printf("\033[0m\033[1;37m%s\037[0m", "\\\\  //");
        printf("\033[0m\033[1;36m%s\033[0m", "     //\n");
        printf("\033[0m\033[1;36m%s\033[0m", " \\\\    ");
        printf("\033[0m\033[1;37m%s\037[0m", "\\\\//");
        printf("\033[0m\033[1;36m%s\033[0m", "     //\n");
        printf("\033[0m\033[1;36m%s\033[0m", "  \\\\   ");
        printf("\033[0m\033[1;37m%s\037[0m", "//\\\\");
        printf("\033[0m\033[1;36m%s\033[0m", "    //     Seraphina\n");
        printf("\033[0m\033[1;36m%s\033[0m", "   \\\\  ");
        printf("\033[0m\033[1;37m%s\037[0m", "//  \\\\");
        printf("\033[0m\033[1;36m%s\033[0m", "  //\n");
        printf("\033[0m\033[1;34m%s\033[0m", "     ==      ==\n\n");
#endif

        /*
        UI Appearance:

        =======      =======
        \\     \\  //     //
         \\     \\//     //
          \\    //\\    //     Seraphina
           \\  //  \\  //
             ==      ==

        */
    }

    void Evil()
    {
#ifdef _WIN64 // Compatible with Microsoft Windows
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
    }

    void NNUE_Test()
    {
        Board board;
        board.ClearBoard();
        board.initCuckoo();
        board.initSQbtw();
        board.initZobrist();
        board.parseFEN(START_FEN);

        Move move = setMove(Square::SQ_E2, Square::SQ_E4, PieceType::WHITE_PAWN, MoveType::NORMAL);

        NNUE::init();
        std::cout << Evaluate(board, move) << "\n";
    }
}

int main()
{
    Seraphina::UI(); // Print Engine's Name, Logo & Version
    // Seraphina::Evil(); // Set Engine's Priority to High if "EVIL" is defined
    Seraphina::NNUE_Test();
    while(true);
}