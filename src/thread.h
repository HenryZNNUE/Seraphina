// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Seraphina
{
	// https://github.com/official-stockfish/Stockfish/blob/master/src/thread_win32_osx.h
	// https://stackoverflow.com/questions/13871763/how-to-set-the-stacksize-with-c11-stdthread
#if defined (__MINGW64__)
#include <functional>
#include <pthread.h>

	class Std_Thread
	{
		pthread_t std_thread;
		std::function<void()> func;

	public:
		void join()
		{
			pthread_join(std_thread, 0);
		}

		static void* nothing(void* v)
		{
			std::function<void()>* func
				= reinterpret_cast<std::function<void()>*>(v);
			(*func)();
			return nullptr;
		}

		Std_Thread(std::function<void()> f) : func(f)
		{
			pthread_attr_t attribute;
			pthread_attr_init(&attribute);
			pthread_attr_setstacksize(&attribute, 8 * 1024 * 1024);
			pthread_create(&std_thread, &attribute, nothing, &func);
		}
	};
#else
	using Std_Thread = std::thread;
#endif

	typedef struct Thread Thread;

	class ThreadPool
	{
		std::vector<Thread*> threads;
		int num = 1;

		std::atomic_bool ponder = true;
		std::atomic_bool stop = false;

	public:
		uint64_t get_nodes();
		uint64_t get_tbhits();

		void set_threads(int n);
		void kill_threads();
		void wait(Thread& thread);
		void wait();
		void wakeup();

		Thread* main_thread();
	};
}