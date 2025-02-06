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
#include <chrono>

#include "uci.h"
#include "bitboard.h"
#include "movegen.h"

namespace Seraphina
{
    // Fancy Terminal
    // Inspired by Lc0
    void UI()
    {
        bool unix_style;

#ifdef _WIN64
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(h, &mode);
        unix_style = SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
        unix_style = true;
#endif
        std::cout << Bold(unix_style) << Cyan(unix_style)
            << " __\n";
        std::cout
            << "(_  _ _ _  _ |_ . _  _\n";
        std::cout << Bold(unix_style) << Cyan(unix_style)
            << "__)(-| (_||_)| )|| )(_|   "
            << Blue(unix_style) << Version << " by " << Author << "\n";
        std::cout << Bold(unix_style) << Cyan(unix_style)
            << "          |\n\n" << Vanilla(unix_style);
    }

    void Evil()
    {
#if defined _WIN64 && defined EVIL // Compatible with Microsoft Windows
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
    }
}

int main()
{
    Seraphina::UI(); // Print Engine's Name, Logo & Version
    Seraphina::Evil(); // Set Engine's Priority to High if "EVIL" is defined
	// Seraphina::MoveGenTest(); // Test Move Generation
    std::cin.get();
}